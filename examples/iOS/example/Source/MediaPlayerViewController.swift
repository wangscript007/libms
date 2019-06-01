//
//  MediaPlayerViewController.swift
//  example
//
//  Created by Jianguo Wu on 2019/5/28.
//  Copyright © 2019 wujianguo. All rights reserved.
//

import UIKit
import AVKit

class MediaPlayerViewController: UIViewController {

    var items = [MediaItem]()
    var index = 0
    var currentPlayResource: MediaServer.MediaResource? = nil
    var preloadResource: MediaServer.MediaResource? = nil
    
    var playerViewController: AVPlayerViewController!
    
    deinit {
        stopPreloadTask()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()

        updateToolBarStatus()
    }
    
    // MARK: - Player

    func play() {
        stopPreloadTask()
        let item = items[index]
        title = item.name
        currentPlayResource = MediaServer.generateResource(url: item.url)
        let player = AVPlayer(url: currentPlayResource!.playURL)
        playerViewController.player = player
        player.play()        
    }
    
    func stopPreloadTask() {
        guard let preloadResource = preloadResource else {
            return
        }
        
        MediaServer.stopTask(resource: preloadResource)
        self.preloadResource = nil
    }

    // MARK: - Status
//    @IBOutlet weak var statusLabel: UILabel!
    
    
    // MARK: - ToolBar
    
    func updateToolBarStatus() {
        previousBarButton.isEnabled = index > 0
        nextBarButton.isEnabled = index < items.count - 1
    }
    
    
    // MARK: - Actions
    
    @IBOutlet weak var previousBarButton: UIBarButtonItem!
    
    @IBOutlet weak var nextBarButton: UIBarButtonItem!
    
    
    @IBAction func didClickPreviousBarButton(_ sender: UIBarButtonItem) {
        defer {
            updateToolBarStatus()
        }
        guard index > 0 else {
            return
        }
        index -= 1
        play()
    }
    
    @IBAction func didClickNextBarButton(_ sender: UIBarButtonItem) {
        defer {
            updateToolBarStatus()
        }
        guard index < items.count - 1 else {
            return
        }
        index += 1
        play()
    }
    
    @IBAction func didClickLoadNextBarButton(_ sender: UIBarButtonItem) {
        guard index < items.count - 1 else {
            return
        }
        stopPreloadTask()
        let resource = MediaServer.generateResource(url: items[index+1].url)
        MediaServer.startTask(resource: resource)
        preloadResource = resource
    }
    
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        if let vc = segue.destination as? AVPlayerViewController {
            playerViewController = vc
            play()
        }
    }
    
    
}
