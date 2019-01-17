/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
#pragma once

#include <memory>

#include <folly/Optional.h>
#include <folly/Range.h>
#include <folly/io/async/SSLContext.h>
#include <mcrouter/lib/network/FizzContextProvider.h>
#include <mcrouter/lib/network/SecurityOptions.h>
#include <wangle/client/ssl/SSLSessionCallbacks.h>
#include <wangle/ssl/TLSTicketKeySeeds.h>

namespace folly {
class SSLContext;
} // folly

namespace facebook {
namespace memcache {

class SSLContextConfig;

class ClientSSLContext : public folly::SSLContext {
 public:
  explicit ClientSSLContext(wangle::SSLSessionCallbacks& cache)
      : cache_(cache) {
    wangle::SSLSessionCallbacks::attachCallbacksToContext(getSSLCtx(), &cache_);
  }

  virtual ~ClientSSLContext() override {
    wangle::SSLSessionCallbacks::detachCallbacksFromContext(
        getSSLCtx(), &cache_);
  }

  wangle::SSLSessionCallbacks& getCache() {
    return cache_;
  }

 private:
  // In our usage, cache_ is a LeakySingleton so the raw reference is safe.
  wangle::SSLSessionCallbacks& cache_;
};

/**
 * Determine if SSL Contexts are thread safe.  Depending on the OpenSSL version,
 * certain locks may be disabled that prevent usage of contexts across threads.
 * This can happen if handshakes are being offloaded to a different thread pool.
 */
bool sslContextsAreThreadSafe();

/**
 * The following methods return thread local managed SSL Contexts.  Contexts are
 * reloaded on demand if they are 30 minutes old on a per thread basis.
 */
FizzContextAndVerifier getFizzClientConfig(const SecurityOptions& opts);

/**
 * Get a context used for client connections.  If opts has an empty CA path, the
 * context will be configured to verify server ceritifcates against the CA.
 * Cert paths for pemCertPath and pemKeyPath may be empty.
 * Client contexts are cached for 24 hours and keyed off of various members in
 * opts.
 */
std::shared_ptr<folly::SSLContext> getClientContext(
    const SecurityOptions& opts,
    SecurityMech mech);

using ServerContextPair = std::pair<
    std::shared_ptr<folly::SSLContext>,
    std::shared_ptr<fizz::server::FizzServerContext>>;

/**
 * Get a context used for accepting ssl connections.  All paths must not be
 * empty.
 * If requireClientCerts is true, clients that do not present a client cert
 * during the handshake will be rejected.
 */
ServerContextPair getServerContexts(
    folly::StringPiece pemCertPath,
    folly::StringPiece pemKeyPath,
    folly::StringPiece pemCaPath,
    bool requireClientCerts,
    folly::Optional<wangle::TLSTicketKeySeeds> seeds);

} // memcache
} // facebook
