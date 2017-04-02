#include "curl_fun.h"
#include "parameters.h"

#include "curl/curl.h"
#include "jansson.h"

#include <unistd.h>


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

json_t* getJsonFromUrl(Parameters& params, std::string url, std::string postFields,
                       bool getRequest)
{
  if (!params.cacert.empty())
    curl_easy_setopt(params.curl, CURLOPT_CAINFO, params.cacert.c_str());

  curl_easy_setopt(params.curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(params.curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(params.curl, CURLOPT_TIMEOUT, 20L);
  curl_easy_setopt(params.curl, CURLOPT_ENCODING, "gzip");
  if (!postFields.empty())
    curl_easy_setopt(params.curl, CURLOPT_POSTFIELDS, postFields.c_str());

  curl_easy_setopt(params.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  std::string readBuffer;
  curl_easy_setopt(params.curl, CURLOPT_WRITEDATA, &readBuffer);
  curl_easy_setopt(params.curl, CURLOPT_DNS_CACHE_TIMEOUT, 3600);
  // Some calls must be GET requests
  if (getRequest)
    curl_easy_setopt(params.curl, CURLOPT_CUSTOMREQUEST, "GET");

  goto curl_state;

retry_state:
  sleep(2.0);
  readBuffer.clear();
  curl_easy_setopt(params.curl, CURLOPT_DNS_CACHE_TIMEOUT, 0);

curl_state:
  CURLcode resCurl = curl_easy_perform(params.curl);
  if (resCurl != CURLE_OK)
  {
    *params.logFile << "Error with cURL: " << curl_easy_strerror(resCurl) << '\n'
                    << "  URL: " << url << '\n'
                    << "  Retry in 2 sec..." << std::endl;

    goto retry_state;
  }

/* documentation label */
// json_state:
  json_error_t error;
  json_t *root = json_loads(readBuffer.c_str(), 0, &error);
  if (!root)
  {
    *params.logFile << "Error with JSON:\n" << error.text << '\n'
                    << "Buffer:\n" << readBuffer << '\n'
                    << "Retrying..." << std::endl;

    goto retry_state;
  }

  // Change back to POST request
  if (getRequest)
    curl_easy_setopt(params.curl, CURLOPT_CUSTOMREQUEST, "POST");

  return root;
}
