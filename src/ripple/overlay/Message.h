//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#ifndef RIPPLE_OVERLAY_MESSAGE_H_INCLUDED
#define RIPPLE_OVERLAY_MESSAGE_H_INCLUDED

#include "ripple.pb.h"
    
#include <memory>

namespace ripple {

// VFALCO NOTE If we forward declare Message and write out shared_ptr
//             instead of using the in-class typedef, we can remove the entire
//             ripple.pb.h from the main headers.
//

// packaging of messages into length/type-prepended buffers
// ready for transmission.
//
// Message implements simple "packing" of protocol buffers Messages into
// a string prepended by a header specifying the message length.
// MessageType should be a Message class generated by the protobuf compiler.
//

class Message : public std::enable_shared_from_this <Message>
{
public:
    typedef std::shared_ptr<Message> pointer;

public:
    /** Number of bytes in a message header.
    */
    static size_t const kHeaderBytes = 6;

    Message (::google::protobuf::Message const& message, int type);

    /** Retrieve the packed message data. */
    std::vector <uint8_t> const&
    getBuffer () const
    {
        return mBuffer;
    }

    /** Determine bytewise equality. */
    bool operator == (Message const& other) const;

    /** Calculate the length of a packed message. */
    static unsigned getLength (std::vector <uint8_t> const& buf);

    /** Determine the type of a packed message. */
    static int getType (std::vector <uint8_t> const& buf);

private:
    // Encodes the size and type into a header at the beginning of buf
    //
    void encodeHeader (unsigned size, int type);

    std::vector <uint8_t> mBuffer;
};

}

#endif


