//
//  MediaPlayer.swift
//  example
//
//  Created by Jianguo Wu on 2019/2/18.
//  Copyright © 2019 libms. All rights reserved.
//

import Foundation
import AVFoundation

class MediaPlayer {
    
    let player: AVPlayer
    
    init(item: MediaItem) {
        player = AVPlayer(url: item.url)
    }
    
}
