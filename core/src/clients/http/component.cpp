#include <clients/http/component.hpp>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/statistics_storage.hpp>
#include <taxi_config/storage/component.hpp>

#include <clients/http/client.hpp>
#include <clients/http/config.hpp>
#include <clients/http/destination_statistics_json.hpp>
#include <clients/http/statistics.hpp>

namespace components {

namespace {
const auto kDestinationMetricsAutoMaxSizeDefault = 100;
}

HttpClient::HttpClient(const ComponentConfig& component_config,
                       const ComponentContext& context)
    : LoggableComponentBase(component_config, context),
      taxi_config_component_(context.FindComponent<components::TaxiConfig>()) {
  auto config = taxi_config_component_.GetBootstrap();
  const auto& http_config = config->Get<clients::http::Config>();
  size_t threads = http_config.threads;

  http_client_ = clients::http::Client::Create(threads);
  http_client_->SetDestinationMetricsAutoMaxSize(
      component_config.ParseInt("destination-metrics-auto-max-size",
                                kDestinationMetricsAutoMaxSizeDefault));

  OnConfigUpdate(config);

  subscriber_scope_ = taxi_config_component_.AddListener(
      this, "http_client",
      &HttpClient::OnConfigUpdate<taxi_config::FullConfigTag>);

  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      "httpclient",
      [this](const utils::statistics::StatisticsRequest& /*request*/) {
        return ExtendStatistics();
      });
}

HttpClient::~HttpClient() {
  statistics_holder_.Unregister();
  subscriber_scope_.Unsubscribe();
}

clients::http::Client& HttpClient::GetHttpClient() {
  if (!http_client_) {
    LOG_ERROR() << "Asking for http client after components::HttpClient "
                   "destructor is called.";
    logging::LogFlush();
    abort();
  }
  return *http_client_;
}

template <typename ConfigTag>
void HttpClient::OnConfigUpdate(
    const std::shared_ptr<taxi_config::BaseConfig<ConfigTag>>& config) {
  const auto& http_client_config =
      config->template Get<clients::http::Config>();
  http_client_->SetConnectionPoolSize(http_client_config.connection_pool_size);
}

formats::json::Value HttpClient::ExtendStatistics() {
  auto json =
      clients::http::PoolStatisticsToJson(http_client_->GetPoolStatistics());
  json["destinations"] = clients::http::DestinationStatisticsToJson(
      http_client_->GetDestinationStatistics());
  return json.ExtractValue();
}

}  // namespace components
