#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

using namespace storages::mongo;

UTEST(Pool, CollectionAccess) {
  static const std::string kSysVerCollName = "system.version";
  static const std::string kNonexistentCollName = "nonexistent";

  // this database always exists
  auto admin_pool = MakeTestsuiteMongoPool("admin");
  // this one should not exist
  auto test_pool = MakeTestsuiteMongoPool("pool_test");

  EXPECT_TRUE(admin_pool.HasCollection(kSysVerCollName));
  EXPECT_NO_THROW(admin_pool.GetCollection(kSysVerCollName));

  EXPECT_FALSE(test_pool.HasCollection(kSysVerCollName));
  EXPECT_NO_THROW(test_pool.GetCollection(kSysVerCollName));

  EXPECT_FALSE(admin_pool.HasCollection(kNonexistentCollName));
  EXPECT_NO_THROW(admin_pool.GetCollection(kNonexistentCollName));

  EXPECT_FALSE(test_pool.HasCollection(kNonexistentCollName));
  EXPECT_NO_THROW(test_pool.GetCollection(kNonexistentCollName));
}

UTEST(Pool, ConnectionFailure) {
  // constructor should not throw
  Pool bad_pool("bad", "mongodb://%2Fnonexistent.sock/bad",
                {"bad", PoolConfig::DriverImpl::kMongoCDriver});
  EXPECT_THROW(bad_pool.HasCollection("test"), ClusterUnavailableException);
}

UTEST(Pool, Limits) {
  PoolConfig limited_config{"limited", PoolConfig::DriverImpl::kMongoCDriver};
  limited_config.max_size = 1;
  auto limited_pool = MakeTestsuiteMongoPool("limits_test", limited_config);

  std::vector<formats::bson::Document> docs;
  /// large enough to not fit into a single batch
  for (int i = 0; i < 150; ++i) {
    docs.push_back(formats::bson::MakeDoc("_id", i));
  }
  limited_pool.GetCollection("test").InsertMany(std::move(docs));

  auto cursor = limited_pool.GetCollection("test").Find({});

  auto second_find = engine::impl::Async(
      [&limited_pool] { limited_pool.GetCollection("test").Find({}); });
  EXPECT_THROW(second_find.Get(), MongoException);
}
