//
//  MediaItem.swift
//  example
//
//  Created by Jianguo Wu on 2019/5/28.
//  Copyright © 2019 wujianguo. All rights reserved.
//


import Foundation

struct MediaItem: Decodable {
    let name: String
    let cover: URL
    let url: URL
    let duration: Int
}
