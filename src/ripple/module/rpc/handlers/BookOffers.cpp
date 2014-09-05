//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2014 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

namespace ripple {

static void getBookPageAll (RPC::Context& context,
                    const unsigned int all_book, Json::Value& jvResult,
                    const unsigned int iLimit);

static void getBookPageByMap (RPC::Context& context,
                    Json::Value& jvResult,
                    const unsigned int iLimit);

Json::Value doBookOffers (RPC::Context& context)
{
    // VFALCO TODO Here is a terrible place for this kind of business
    //             logic. It needs to be moved elsewhere and documented,
    //             and encapsulated into a function.
    if (getApp().getJobQueue ().getJobCountGE (jtCLIENT) > 200)
        return rpcError (rpcTOO_BUSY);

    Ledger::pointer lpLedger;
    Json::Value jvResult (
        RPC::lookupLedger (context.params_, lpLedger, context.netOps_));

    if (!lpLedger)
        return jvResult;

    if (context.params_.isMember("currencies"))
    {

        unsigned int const all_book (context.params_.isMember ("all_book")
            ? context.params_ ["all_book"].asUInt ()
            : 0);

        unsigned int const iLimit (context.params_.isMember ("limit")
            ? context.params_ ["limit"].asUInt ()
            : 0);

        Currency src_currency;
        Currency dst_currency;
        WriteLog (lsINFO, RPCHandler) << "whans process all book size";
        getBookPageAll (context, all_book, jvResult, iLimit);
        return jvResult;
    }

    if (context.params_.isMember("map_pays"))
    {
        unsigned int const iLimit (context.params_.isMember ("limit")
            ? context.params_ ["limit"].asUInt ()
            : 0);

        WriteLog (lsWARNING, RPCHandler) << "whans process getBookPageByMap";
        getBookPageByMap (context, jvResult, iLimit);
        return jvResult;
    }

    if (!context.params_.isMember ("taker_pays"))
        return RPC::missing_field_error ("taker_pays");

    if (!context.params_.isMember ("taker_gets"))
        return RPC::missing_field_error ("taker_gets");

    if (!context.params_["taker_pays"].isObject ())
        return RPC::object_field_error ("taker_pays");

    if (!context.params_["taker_gets"].isObject ())
        return RPC::object_field_error ("taker_gets");

    Json::Value const& taker_pays (context.params_["taker_pays"]);

    if (!taker_pays.isMember ("currency"))
        return RPC::missing_field_error ("taker_pays.currency");

    if (! taker_pays ["currency"].isString ())
        return RPC::expected_field_error ("taker_pays.currency", "string");

    Json::Value const& taker_gets = context.params_["taker_gets"];

    if (! taker_gets.isMember ("currency"))
        return RPC::missing_field_error ("taker_gets.currency");

    if (! taker_gets ["currency"].isString ())
        return RPC::expected_field_error ("taker_gets.currency", "string");

    Currency pay_currency;

    if (!to_currency (pay_currency, taker_pays ["currency"].asString ()))
    {
        WriteLog (lsINFO, RPCHandler) << "Bad taker_pays currency.";
        return RPC::make_error (rpcSRC_CUR_MALFORMED,
            "Invalid field 'taker_pays.currency', bad currency.");
    }

    Currency get_currency;

    if (!to_currency (get_currency, taker_gets ["currency"].asString ()))
    {
        WriteLog (lsINFO, RPCHandler) << "Bad taker_gets currency.";
        return RPC::make_error (rpcDST_AMT_MALFORMED,
            "Invalid field 'taker_gets.currency', bad currency.");
    }

    Account pay_issuer;

    if (taker_pays.isMember ("issuer"))
    {
        if (! taker_pays ["issuer"].isString())
            return RPC::expected_field_error ("taker_pays.issuer", "string");

        if (!to_issuer(
            pay_issuer, taker_pays ["issuer"].asString ()))
            return RPC::make_error (rpcSRC_ISR_MALFORMED,
                "Invalid field 'taker_pays.issuer', bad issuer.");

        if (pay_issuer == noAccount ())
            return RPC::make_error (rpcSRC_ISR_MALFORMED,
                "Invalid field 'taker_pays.issuer', bad issuer account one.");
    }
    else
    {
        pay_issuer = xrpAccount ();
    }

    if (isXRP (pay_currency) && ! isXRP (pay_issuer))
        return RPC::make_error (
            rpcSRC_ISR_MALFORMED, "Unneeded field 'taker_pays.issuer' for "
            "XRP currency specification.");

    if (!isXRP (pay_currency) && isXRP (pay_issuer))
        return RPC::make_error (rpcSRC_ISR_MALFORMED,
            "Invalid field 'taker_pays.issuer', expected non-XRP issuer.");

    Account get_issuer;

    if (taker_gets.isMember ("issuer"))
    {
        if (! taker_gets ["issuer"].isString())
            return RPC::expected_field_error ("taker_gets.issuer", "string");

        if (! to_issuer (
            get_issuer, taker_gets ["issuer"].asString ()))
            return RPC::make_error (rpcDST_ISR_MALFORMED,
                "Invalid field 'taker_gets.issuer', bad issuer.");

        if (get_issuer == noAccount ())
            return RPC::make_error (rpcDST_ISR_MALFORMED,
                "Invalid field 'taker_gets.issuer', bad issuer account one.");
    }
    else
    {
        get_issuer = xrpAccount ();
    }


    if (isXRP (get_currency) && ! isXRP (get_issuer))
        return RPC::make_error (rpcDST_ISR_MALFORMED,
            "Unneeded field 'taker_gets.issuer' for "
                               "XRP currency specification.");

    if (!isXRP (get_currency) && isXRP (get_issuer))
        return RPC::make_error (rpcDST_ISR_MALFORMED,
            "Invalid field 'taker_gets.issuer', expected non-XRP issuer.");

    RippleAddress raTakerID;

    if (context.params_.isMember ("taker"))
    {
        if (! context.params_ ["taker"].isString ())
            return RPC::expected_field_error ("taker", "string");

        if (! raTakerID.setAccountID (context.params_ ["taker"].asString ()))
            return RPC::invalid_field_error ("taker");
    }
    else
    {
        raTakerID.setAccountID (noAccount());
    }

    if (pay_currency == get_currency && pay_issuer == get_issuer)
    {
        WriteLog (lsINFO, RPCHandler) << "taker_gets same as taker_pays.";
        return RPC::make_error (rpcBAD_MARKET);
    }

    if (context.params_.isMember ("limit") &&
        !context.params_ ["limit"].isIntegral())
    {
        return RPC::expected_field_error ("limit", "integer");
    }

    unsigned int const iLimit (context.params_.isMember ("limit")
        ? context.params_ ["limit"].asUInt ()
        : 0);

    bool const bProof (context.params_.isMember ("proof"));

    Json::Value const jvMarker (context.params_.isMember ("marker")
        ? context.params_["marker"]
        : Json::Value (Json::nullValue));

    context.netOps_.getBookPage (
        lpLedger,
        {{pay_currency, pay_issuer}, {get_currency, get_issuer}},
        raTakerID.getAccountID (), bProof, iLimit, jvMarker, jvResult);

    context.loadType_ = Resource::feeMediumBurdenRPC;

    return jvResult;
}

static void getBookPageAll (RPC::Context& context,
                    const unsigned int all_book, Json::Value& jvResult,
                    const unsigned int iLimit)
//static void getBookPageAll (RPC::Context& context, const unsigned int iLimit, Json::Value& jvResult)
{
    OrderBookDB::IssueToOrderBook sourceMap = getApp().getOrderBookDB ().getOrderBookSourceMap();
    OrderBookDB::IssueToOrderBook::iterator srcIt = sourceMap.begin();
    auto lpLedger = getApp().getLedgerMaster ().getPublishedLedger ();
    if (!lpLedger)
        return;

    const Json::Value jvMarker = Json::Value (Json::nullValue);
    jvResult[jss::offers] = Json::Value (Json::arrayValue);

    RippleAddress   raTakerID;
    raTakerID.setAccountID (noAccount());

    Currency src_currency;
    Currency dst_currency;
    std::string currency_1;
    std::string currency_2;

    for (; srcIt != sourceMap.end(); ++srcIt)
    {
        for (auto ob : srcIt->second)
        {
            Json::Value jvOffer  = Json::Value (Json::nullValue);
            WriteLog (lsWARNING, RPCHandler) << "in for getBookPageAll whans book: " << ob->book();

            if (!all_book)
            {
                for (auto& it1: context.params_["currencies"])
                {
                    for (auto& it2: context.params_["currencies"])
                    {
                        currency_1 = it1.asString ();
                        currency_2 = it2.asString ();

                        if (!to_currency (src_currency, currency_1))
                        {
                            WriteLog (lsINFO, RPCHandler) << "whans Bad src currency.";
                        }

                        if (!to_currency (dst_currency, currency_2))
                        {
                            WriteLog (lsINFO, RPCHandler) << "whans Bad src currency.";
                        }
                        if(ob->book().in.currency == src_currency
                            && ob->book().out.currency == dst_currency)
                        {
                            context.netOps_.getBookPage (
                                lpLedger, ob->book(), raTakerID.getAccountID (), false, iLimit,
                                jvMarker, jvOffer);
                            jvResult[jss::offers].append(jvOffer[jss::offers]);
                        }

                    }
                }
            }
            else
            {
                context.netOps_.getBookPage (
                    lpLedger, ob->book(), raTakerID.getAccountID (), false, iLimit,
                    jvMarker, jvOffer);
                jvResult[jss::offers].append(jvOffer[jss::offers]);
            }

        }
    }
    WriteLog (lsWARNING, RPCHandler) << "getBookPageALl whans book sizes: ";

}

static void getBookPageByMap (RPC::Context& context,
                    Json::Value& jvResult,
                    const unsigned int iLimit)
{
    auto lpLedger = getApp().getLedgerMaster ().getPublishedLedger ();
    if (!lpLedger)
        return;

    const Json::Value jvMarker = Json::Value (Json::nullValue);
    jvResult[jss::offers] = Json::Value (Json::arrayValue);

    Account pay_issuer;
    Account get_issuer;
    Currency pay_currency;
    Currency get_currency;
    RippleAddress   raTakerID;
    raTakerID.setAccountID (noAccount());

    for (auto& it1: context.params_["map_pays"])
    {
        WriteLog (lsWARNING, RPCHandler) << "whans in getBookPageByMap for";
        int count = 0;
        Json::Value jvOffer  = Json::Value (Json::nullValue);
        for (auto& it2: it1)
        {
            if (count == 0)
                to_currency(pay_currency, it2.asString());
            if (count == 1)
                to_issuer(pay_issuer, it2.asString());
            if (count == 2)
                to_currency(get_currency, it2.asString());
            if (count == 3)
                to_issuer(get_issuer, it2.asString());
            count++;
        }

        WriteLog (lsWARNING, RPCHandler) << "whans in getBookPageByMap map: " << pay_currency
                        << "/" << pay_issuer << "," << get_currency << "/"<<get_issuer;
        context.netOps_.getBookPage (
            lpLedger,
            {{pay_currency, pay_issuer}, {get_currency, get_issuer}},
            raTakerID.getAccountID (), false, iLimit, jvMarker, jvOffer);

        jvResult[jss::offers].append(jvOffer[jss::offers]);

        context.netOps_.getBookPage (
            lpLedger,
            {{get_currency, get_issuer}, {pay_currency, pay_issuer}},
            raTakerID.getAccountID (), false, iLimit, jvMarker, jvOffer);

        jvResult[jss::offers].append(jvOffer[jss::offers]);
    }
}

} // ripple
