/*
 *  Copyright (c) 2016, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

#include "mcrouter/lib/fbi/cpp/LogFailure.h"
#include "mcrouter/lib/McReply.h"
#include "mcrouter/lib/McRequest.h"
#include "mcrouter/lib/network/TypedThriftMessage.h"

namespace facebook { namespace memcache {

template <class Callback>
ClientMcParser<Callback>::ClientMcParser(Callback& cb,
                                         size_t requestsPerRead,
                                         size_t minBufferSize,
                                         size_t maxBufferSize,
                                         mc_protocol_t protocol,
                                         const bool useJemallocNodumpAllocator)
  : parser_(*this,
            requestsPerRead,
            minBufferSize,
            maxBufferSize,
            useJemallocNodumpAllocator),
    callback_(cb) {
}

template <class Callback>
std::pair<void*, size_t> ClientMcParser<Callback>::getReadBuffer() {
  if (shouldReadToAsciiBuffer()) {
    return asciiParser_.getReadBuffer();
  } else {
    return parser_.getReadBuffer();
  }
}

template <class Callback>
bool ClientMcParser<Callback>::readDataAvailable(size_t len) {
  if (shouldReadToAsciiBuffer()) {
    asciiParser_.readDataAvailable(len);
    return true;
  } else {
    return parser_.readDataAvailable(len);
  }
}

template <class Callback>
template <class Request>
void ClientMcParser<Callback>::expectNext() {
  if (parser_.protocol() == mc_ascii_protocol) {
    asciiParser_.initializeReplyParser<Request>();
    replyForwarder_ =
      &ClientMcParser<Callback>::forwardAsciiReply<Request>;
  } else if (parser_.protocol() == mc_umbrella_protocol ||
             parser_.protocol() == mc_caret_protocol) {
    umbrellaForwarder_ =
      &ClientMcParser<Callback>::forwardUmbrellaReply<Request>;
  }
}

template <class Callback>
void ClientMcParser<Callback>::replyReadyHelper(McReply&& reply,
                                                uint64_t reqid) {
  parser_.reportMsgRead();
  callback_.replyReady(std::move(reply), reqid);
}

template <class Callback>
template <class Request>
void ClientMcParser<Callback>::forwardAsciiReply() {
  parser_.reportMsgRead();
  callback_.replyReady(
    asciiParser_.getReply<ReplyT<Request>>(), 0 /* reqId */);
  replyForwarder_ = nullptr;
}

template <class Callback>
template <class Request>
typename std::enable_if<!IsCustomRequest<Request>::value, void>::type
ClientMcParser<Callback>::forwardUmbrellaReply(
    const UmbrellaMessageInfo& info,
    const folly::IOBuf& buffer,
    uint64_t reqId) {

  if (info.version == UmbrellaVersion::BASIC) {
    auto reply = umbrellaParseReply<Request>(
        buffer,
        buffer.data(),
        info.headerSize,
        buffer.data() + info.headerSize,
        info.bodySize);
    callback_.replyReady(std::move(reply), reqId);
  } else {
    ReplyT<Request> reply;

    folly::IOBuf trim;
    buffer.cloneOneInto(trim);
    trim.trimStart(info.headerSize);

    // Task: 8257655 - Conversion should be moved to ProxyDestination
    converter_.dispatchTypedRequest(info, trim, reply);
    callback_.replyReady(std::move(reply), reqId);
  }
}

template <class Callback>
template <class Request>
typename std::enable_if<IsCustomRequest<Request>::value, void>::type
ClientMcParser<Callback>::forwardUmbrellaReply(
    const UmbrellaMessageInfo& info,
    const folly::IOBuf& buffer,
    uint64_t reqId) {

  if (info.version == UmbrellaVersion::BASIC) {
    auto reply = umbrellaParseReply<Request>(
        buffer,
        buffer.data(),
        info.headerSize,
        buffer.data() + info.headerSize,
        info.bodySize);
    callback_.replyReady(std::move(reply), reqId);
  } else {
    ReplyT<Request> reply;
    folly::IOBuf trim;
    buffer.cloneOneInto(trim);
    trim.trimStart(info.headerSize);

    apache::thrift::CompactProtocolReader reader;
    reader.setInput(&trim);
    reply.read(&reader);

    callback_.replyReady(std::move(reply), reqId);
  }
}

template <class Callback>
bool ClientMcParser<Callback>::umMessageReady(const UmbrellaMessageInfo& info,
                                              const folly::IOBuf& buffer) {
  if (UNLIKELY((parser_.protocol() != mc_umbrella_protocol) &&
                (parser_.protocol() != mc_caret_protocol))) {
    std::string reason =
        folly::sformat("Expected {} protocol, but received umbrella!",
                       mc_protocol_to_string(parser_.protocol()));
    callback_.parseError(mc_res_local_error, reason);
    return false;
  }

  try {
    size_t reqId;
    if (info.version == UmbrellaVersion::BASIC) {
      reqId = umbrellaDetermineReqId(buffer.data(), info.headerSize);
    } else {
      reqId = info.reqId;
    }
    if (callback_.nextReplyAvailable(reqId)) {
      (this->*umbrellaForwarder_)(info, buffer, reqId);
    }
    // Consume the message, but don't fail.
    return true;
  } catch (const std::runtime_error& e) {
    std::string reason(
      std::string("Error parsing Umbrella message: ") + e.what());
    LOG(ERROR) << reason;
    callback_.parseError(mc_res_local_error, reason);
    return false;
  }
}

template <class Callback>
void ClientMcParser<Callback>::handleAscii(folly::IOBuf& readBuffer) {
  if (UNLIKELY(parser_.protocol() != mc_ascii_protocol)) {
    std::string reason(
      folly::sformat("Expected {} protocol, but received ASCII!",
                     mc_protocol_to_string(parser_.protocol())));
    callback_.parseError(mc_res_local_error, reason);
    return;
  }

  while (readBuffer.length()) {
    if (asciiParser_.getCurrentState() == McAsciiParserBase::State::UNINIT) {
      // Ask the client to initialize parser.
      if (!callback_.nextReplyAvailable(0 /* reqId */)) {
        auto data = reinterpret_cast<const char *>(readBuffer.data());
        std::string reason(folly::sformat(
            "Received unexpected data from remote endpoint: '{}'!",
            folly::cEscape<std::string>(folly::StringPiece(
                data, data + std::min(readBuffer.length(),
                                      static_cast<size_t>(128))))));
        callback_.parseError(mc_res_local_error, reason);
        return;
      }
    }
    switch (asciiParser_.consume(readBuffer)) {
    case McAsciiParserBase::State::COMPLETE:
      (this->*replyForwarder_)();
      break;
    case McAsciiParserBase::State::ERROR:
      callback_.parseError(mc_res_local_error,
                           asciiParser_.getErrorDescription());
      return;
    case McAsciiParserBase::State::PARTIAL:
      // Buffer was completely consumed.
      break;
    case McAsciiParserBase::State::UNINIT:
      // We fed parser some data, it shouldn't remain in State::NONE.
      callback_.parseError(mc_res_local_error,
                           "Sent data to AsciiParser but it remained in "
                           "UNINIT state!");
      return;
    }
  }
}

template <class Callback>
void ClientMcParser<Callback>::parseError(mc_res_t result,
                                          folly::StringPiece reason) {
  callback_.parseError(result, reason);
}

template <class Callback>
bool ClientMcParser<Callback>::shouldReadToAsciiBuffer() const {
  return parser_.protocol() == mc_ascii_protocol &&
         asciiParser_.hasReadBuffer();
}

}}  // facebook::memcache
