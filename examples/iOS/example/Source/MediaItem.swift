//
//  MediaItem.swift
//  example
//
//  Created by Jianguo Wu on 2019/2/18.
//  Copyright © 2019 libms. All rights reserved.
//

import Foundation

struct MediaItem: Decodable {
    let name: String
    let cover: URL
    let url: URL
    let duration: Int
}
