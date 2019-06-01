//
//  MediaServer.swift
//  example
//
//  Created by Jianguo Wu on 2019/5/28.
//  Copyright © 2019 wujianguo. All rights reserved.
//

import Foundation


struct MediaServer {
    
    typealias Completion = (Error?) -> Void

    enum MediaServerError: Error {
        case serverError(Int, String)
        case defaultError
    }
    
    struct MediaResource {
        let originURL: URL
        let playURL: URL
        let resourceId: Int
    }
    
//    enum StorageMode: String, Decodable {
//        case memory
//        case purefile
//        case lbms1file
//    }
//
//    enum PipeStrategy: String, Decodable {
//        case preload
//        case vod
//        case download
//    }
//
//    struct DynamicURLMethod: Decodable {
//        let method: String
//        let params: [String: String]
//    }
//
//    struct DataEncryptMethod: Decodable {
//        let method: String
//        let params: [String: String]
//    }

    struct Reader: Decodable {
        let pos: Int64
        let len: Int64
        let id: String
    }
    
    struct Pipe: Decodable {
        let pos: Int64
        let len: Int64
        let id: String
    }
    
    struct Task: Decodable {
        let readers: [Reader]
        let pipes: [Pipe]
        let id: String
        let url: URL
        let playURL: URL
        let totalLength: Int64
        let completedLength: Int64
        let speed: Int64
        let name: String
//        let pipeStrategy: PipeStrategy
//        let storageMode: StorageMode
//        let downloadMode: Bool
        let bitmap: String?
        
//        let dynamicURLMethod: DynamicURLMethod?
//        let dataEncryptMethod: DataEncryptMethod?
    }
    
    struct Result: Decodable {
        
    }
    
    static func buildURL(endpoint: String) -> URL {
        let base = URL(string: "http://127.0.0.1:\(ms_server_port())")!
        return base.appendingPathComponent(endpoint)
    }
    
    static func start() {
        let c_url = NSTemporaryDirectory().cString(using: .utf8)
        ms_asnyc_start(8090, c_url)
    }
    
    static func generateResource(url: URL) -> MediaResource {
        let c_url = url.absoluteString.cString(using: .utf8)
        let c_path = url.lastPathComponent.cString(using: .utf8)
        var in_param = ms_url_param(url: c_url, path: c_path)
        
        let out_url = UnsafeMutablePointer<Int8>.allocate(capacity: 1024*1024)
        var resource_id: Int32 = 0
        ms_generate_resource(&in_param, out_url, 1024*1024, &resource_id)
        let play_url = String(cString: out_url)
        out_url.deallocate()
        return MediaResource(originURL: url, playURL: URL(string: play_url)!, resourceId: Int(resource_id))
    }
    
//    static func removeResource(resource: MediaResource) {
//        ms_remove_resource(Int32(resource.resourceId))
//    }
    
    static func generateURL(url: URL) -> URL {
        let c_url = url.absoluteString.cString(using: .utf8)
        let c_path = url.lastPathComponent.cString(using: .utf8)
        var in_param = ms_url_param(url: c_url, path: c_path)
        
        let out_url = UnsafeMutablePointer<Int8>.allocate(capacity: 1024*1024)
        ms_generate_url(&in_param, out_url, 1024*1024)
        let proxy_url = String(cString: out_url)
        out_url.deallocate()
        return URL(string: proxy_url)!
    }
    
    static func startTask(resource: MediaResource,
                          complete: Completion? = nil) {
        var request = URLRequest(url: buildURL(endpoint: "/api/tasks"))
        request.httpMethod = "POST"
        let body = "resourceId=\(resource.resourceId)"
        request.httpBody = body.data(using: .utf8)
        response(request: request, success: { (result: Result) in
            complete?(nil)
        }) { (error) in
            complete?(error)
        }
    }
    
    static func updateTask(resource: MediaResource, complete: Completion? = nil) {
        
    }

    static func getTask(resource: MediaResource, complete: ((Task?, Error?) -> Void)? = nil) {
        let request = URLRequest(url: buildURL(endpoint: "/api/tasks/\(resource.resourceId)"))
        response(request: request, success: { (task: Task) in
            complete?(task, nil)
        }) { (error) in
            complete?(nil, error)
        }
    }

    static func stopTask(resource: MediaResource, complete: Completion? = nil) {
        var request = URLRequest(url: buildURL(endpoint: "/api/tasks/\(resource.resourceId)"))
        request.httpMethod = "DELETE"
        response(request: request, success: { (result: Result) in
            complete?(nil)
        }) { (error) in
            complete?(error)
        }
    }
    
    static private func encodeForURL(str: String) -> String {
        return str.addingPercentEncoding(withAllowedCharacters: CharacterSet(charactersIn: "!*'();:@&=+$,/?%#[]{} ").inverted)!
    }
    
    @discardableResult
    static private func response<T: Decodable>(request: URLRequest, success: ((T)->Void)?, failure: ((Error)->Void)?) -> URLSessionDataTask {
        let task = URLSession.shared.dataTask(with: request) { (data, response, err) in
            if let err = err {
                DispatchQueue.main.async {
                    failure?(err)
                }
            }
            if let res = response as? HTTPURLResponse {
                if res.statusCode != 200 {
                    DispatchQueue.main.async {
                        failure?(MediaServerError.serverError(res.statusCode, HTTPURLResponse.localizedString(forStatusCode: res.statusCode)))
                    }
                    return
                }
            }
            if let data = data {
                do {
                    let ret = try JSONDecoder().decode(T.self, from: data)
                    DispatchQueue.main.async {
                        success?(ret)
                    }
                } catch {
                    DispatchQueue.main.async {
                        failure?(error)
                    }
                }
            } else {
                DispatchQueue.main.async {
                    failure?(MediaServerError.defaultError)
                }
            }
        }
        task.resume()
        return task
    }
}
