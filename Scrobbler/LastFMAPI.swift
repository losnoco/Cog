//
//  LastFMAPI.swift
//  Cog
//
//  Created by Leonardo Lobato on 11/08/25.
//

import Foundation

enum LastFMAPIError: Error {
    case failedToCreateUrl
    case invalidResponse
    case statusCode(Int)
    case unknown
    case apiError(String)
}

struct LastFMAuthResponse: Decodable {
    let session: Session
    struct Session: Decodable {
        let name: String
        let key: String
    }
}

struct LastFMErrorResponse: Decodable {
    let error: Int
    let message: String
}

class LastFMAPI {

    let apiKey: String
    let apiSecret: String
    var sessionKey: String? {
        return KeychainHelper.loadSessionKey()
    }

    var isAuthenticated: Bool {
        return sessionKey != nil
    }

    init(apiKey: String, apiSecret: String) {
        self.apiKey = apiKey
        self.apiSecret = apiSecret
    }

    func authenticateMobile(username: String, password: String, completion: @escaping (Result<(sessionKey: String, username: String), Error>) -> Void) {
        let params = [
            "username": username,
            "password": password,
        ]
        parsedRequest("auth.getMobileSession", params: params) { (result: Result<LastFMAuthResponse?, Error>) in
            switch result {
            case .success(let response):
                if let response {
                    completion(.success((sessionKey: response.session.key, username: response.session.name)))
                } else {
                    completion(.failure(LastFMAPIError.invalidResponse))
                }
            case .failure(let error):
                completion(.failure(error))
            }
        }
    }

    // TODO: parse response, async/await is only available in 10.15
    func parsedRequest<T: Decodable>(_ method: String, httpMethod: String = "POST", params: [String: String] = [:], callback: ((Result<T?, Error>) -> Void)? = nil) {
        self.apiRequest(method, httpMethod: httpMethod, params: params) { result in
            switch result {
            case .success(let data):
                guard let data = data else {
                    callback?(.failure(LastFMAPIError.unknown))
                    return
                }
                do {
                    let response = try JSONDecoder().decode(T.self, from: data)
                    callback?(.success(response))
                } catch {
                    callback?(.failure(error))
                }
            case .failure(let error):
                callback?(.failure(error))
            }
        }
    }

    // No response parsing needed
    func request(_ method: String, httpMethod: String = "POST", params: [String: String] = [:], callback: ((Error?) -> Void)? = nil) {
        self.apiRequest(method, httpMethod: httpMethod, params: params) { result in
            switch result {
            case .success:
                callback?(nil)
            case .failure(let error):
                callback?(error)
            }
        }
    }

    private func apiRequest(_ method: String, httpMethod: String, params: [String: String], callback: ((Result<Data?, Error>) -> Void)? = nil) {
        var requestParams: [String: String?] = params //.mapValues({ $0.addingPercentEncoding(withAllowedCharacters: .urlPathAllowed) })
        requestParams["method"] = method
        requestParams["api_key"] = apiKey
        if let sessionKey {
            requestParams["sk"] = sessionKey
        }
        requestParams["format"] = "json"

        let signedString = requestParams.sorted(by: { $0.key < $1.key }).reduce("") {
            if let value = $1.value, $1.key != "format" {
                return $0 + $1.key + value
            } else {
                return $0
            }
        } + apiSecret

        requestParams["api_sig"] = NSData(data: signedString.data(using: .utf8)!).md5()

        var components = URLComponents(string: "https://ws.audioscrobbler.com/2.0/")!
        if httpMethod == "GET" {
            components.queryItems = requestParams.map { URLQueryItem(name: $0.key, value: $0.value) }
        }

        guard let url = components.url else {
            callback?(.failure(LastFMAPIError.failedToCreateUrl))
            return
        }

        var request = URLRequest(url: url)
        request.httpMethod = httpMethod
        request.setValue("application/x-www-form-urlencoded", forHTTPHeaderField: "Content-Type")
        if httpMethod == "POST" {
            let allowed = CharacterSet.urlQueryAllowed.subtracting(CharacterSet(charactersIn: "+&="))
            let postString: String = requestParams.compactMap({
                guard let value = $0.value,
                      let encodedKey = $0.key.addingPercentEncoding(withAllowedCharacters: allowed),
                      let encodedValue = value.addingPercentEncoding(withAllowedCharacters: allowed)
                else { return nil }
                return encodedKey + "=" + encodedValue
            }).joined(separator: "&")
            request.httpBody = postString.data(using: .utf8)
        }

        URLSession.shared.dataTask(with: request) { data, response, error in
            if let error {
                callback?(.failure(error))
                return
            }
            guard let response = response as? HTTPURLResponse else {
                callback?(.failure(LastFMAPIError.invalidResponse))
                return
            }
            guard response.statusCode == 200 else {
                callback?(.failure(LastFMAPIError.statusCode(response.statusCode)))
                return
            }
            if let data = data,
               let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
               json["error"] != nil,
               let errorResponse = try? JSONDecoder().decode(LastFMErrorResponse.self, from: data) {
                callback?(.failure(LastFMAPIError.apiError(errorResponse.message)))
                return
            }
            callback?(.success(data))
        }.resume()
    }
}
