#pragma once

#include <memory>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace curl {
class form;
}  // namespace curl

namespace clients::http {

class Form final {
 public:
  Form();
  ~Form();

  Form(const Form&) = delete;
  Form(Form&&);
  Form& operator=(const Form&) = delete;
  Form& operator=(Form&&);

  void AddContent(std::string_view key, std::string_view content);
  void AddContent(std::string_view key, std::string_view content,
                  const std::string& content_type);

  void AddBuffer(const std::string& key, const std::string& file_name,
                 const std::shared_ptr<std::string>& buffer);
  void AddBuffer(const std::string& key, const std::string& file_name,
                 const std::shared_ptr<std::string>& buffer,
                 const std::string& content_type);

  curl::form* GetNative() const;

 private:
  std::unique_ptr<curl::form> impl_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
