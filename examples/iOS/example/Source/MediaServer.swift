//
//  MediaServer.swift
//  example
//
//  Created by Jianguo Wu on 2019/5/28.
//  Copyright © 2019 wujianguo. All rights reserved.
//

import Foundation


typealias Completion = (Error?) -> Void

struct MediaServer {
    
    static func baseURL() -> String {
        return "http://127.0.0.1"
    }
    
    static func start() {
        ms_asnyc_start()
    }
    
    static func generateURL(url: URL) -> URL {
        let c_url = url.absoluteString.cString(using: .utf8)
        let c_path = url.lastPathComponent.cString(using: .utf8)
        var in_param = ms_url_param(url: c_url, path: c_path)
        
        let out_url = UnsafeMutablePointer<Int8>.allocate(capacity: 1024*1024)
        ms_generate_url(&in_param, out_url, 1024*1024)
        let proxy_url = String(cString: out_url)
        return URL(string: proxy_url)!
    }
    
    static func startPreload(url: String, complete: Completion? = nil) {
        
    }
    
    static func stopPreload(url: String, complete: Completion? = nil) {
        
    }
}
