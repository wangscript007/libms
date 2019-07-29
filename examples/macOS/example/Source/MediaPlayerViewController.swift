//
//  MediaPlayerViewController.swift
//  example
//
//  Created by Jianguo Wu on 2019/7/27.
//  Copyright © 2019 wujianguo. All rights reserved.
//

import Cocoa
import AVKit
import AVFoundation

class MediaPlayerViewController: NSViewController {

    
    @IBOutlet weak var playerView: AVPlayerView!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        let url = URL(string: "http://vfx.mtime.cn/Video/2019/02/13/mp4/190213103941602230.mp4")!
        let resource = MediaServer.generateResource(url: url)
        let player = AVPlayer(url: resource.playURL)
        playerView.player = player
        playerView.player?.play()
    }
}
