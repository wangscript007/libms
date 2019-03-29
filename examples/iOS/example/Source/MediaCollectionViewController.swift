//
//  MediaCollectionViewController.swift
//  example
//
//  Created by Jianguo Wu on 2019/2/18.
//  Copyright © 2019 libms. All rights reserved.
//

import UIKit
import AVKit

class MediaCollectionViewController: UICollectionViewController {

    var items = [MediaItem]()
    override func viewDidLoad() {
        super.viewDidLoad()

        self.collectionView!.register(MediaCollectionViewCell.self, forCellWithReuseIdentifier: MediaCollectionViewCell.identifier)

        let url = URL(string: "https://raw.githubusercontent.com/wujianguo/libms/master/examples/videos.json")!
        let task = URLSession.shared.dataTask(with: url) { (data, response, error) in
            guard let data = data else {
                return
            }
            let decoder = JSONDecoder()
            if let items = try? decoder.decode([MediaItem].self, from: data) {
                DispatchQueue.main.async {
                    self.items = items
                    self.collectionView.reloadData()
                }
            }
        }
        task.resume()
    }
    
    // MARK: - Player
//    lazy var playerController: AVPlayerViewController = {
//        let controller = AVPlayerViewController()
//        return controller
//    }()

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using [segue destinationViewController].
        // Pass the selected object to the new view controller.
    }
    */

    // MARK: UICollectionViewDataSource

    override func numberOfSections(in collectionView: UICollectionView) -> Int {
        return 1
    }

    override func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return items.count
    }

    override func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: MediaCollectionViewCell.identifier, for: indexPath) as! MediaCollectionViewCell
        cell.item = items[indexPath.row]
        return cell
    }

    // MARK: UICollectionViewDelegate

    override func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        let vc = AVPlayerViewController()
        let item = items[indexPath.row]
        let c_url = UnsafePointer<Int8>(item.url.absoluteString.cString(using: .ascii))
        let c_path = UnsafePointer<Int8>(item.url.lastPathComponent.cString(using: .ascii))
        
        var in_param = ms_url_param(url: c_url, path: c_path)

        let out_url = UnsafeMutablePointer<Int8>.allocate(capacity: 1024*1024)
        ms_generate_url(&in_param, out_url, 1024*1024)
        let proxy_url = String(cString: out_url)
        vc.player = AVPlayer(url: URL(string: proxy_url)!)
        vc.player?.play()
        present(vc, animated: true, completion: nil)
    }
    
    /*
    // Uncomment this method to specify if the specified item should be highlighted during tracking
    override func collectionView(_ collectionView: UICollectionView, shouldHighlightItemAt indexPath: IndexPath) -> Bool {
        return true
    }
    */

    /*
    // Uncomment this method to specify if the specified item should be selected
    override func collectionView(_ collectionView: UICollectionView, shouldSelectItemAt indexPath: IndexPath) -> Bool {
        return true
    }
    */

    /*
    // Uncomment these methods to specify if an action menu should be displayed for the specified item, and react to actions performed on the item
    override func collectionView(_ collectionView: UICollectionView, shouldShowMenuForItemAt indexPath: IndexPath) -> Bool {
        return false
    }

    override func collectionView(_ collectionView: UICollectionView, canPerformAction action: Selector, forItemAt indexPath: IndexPath, withSender sender: Any?) -> Bool {
        return false
    }

    override func collectionView(_ collectionView: UICollectionView, performAction action: Selector, forItemAt indexPath: IndexPath, withSender sender: Any?) {
    
    }
    */

    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
//        UICollectionViewFlowLayout
        return CGSize(width: 120, height: 10)
    }
}
