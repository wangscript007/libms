//
//  UIImageView+Cache.swift
//  example
//
//  Created by Jianguo Wu on 2019/5/28.
//  Copyright © 2019 wujianguo. All rights reserved.
//


import UIKit

private func getAssociatedObject<T>(_ object: Any, _ key: UnsafeRawPointer) -> T? {
    return objc_getAssociatedObject(object, key) as? T
}

private func setRetainedAssociatedObject<T>(_ object: Any, _ key: UnsafeRawPointer, _ value: T) {
    objc_setAssociatedObject(object, key, value, .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
}

private class Box<T> {
    let value: T
    
    init(_ value: T) {
        self.value = value
    }
}

let ImageCache = NSCache<NSURL, UIImage>()

private var imageURLKey: Void?
private var imageTaskKey: Void?

extension UIImageView {
    func setImage(url: URL) {
        if let image = ImageCache.object(forKey: url as NSURL) {
            self.image = image
            return
        }
        self.image = nil
        self.imageURL = url
        self.task?.cancel()
        let task = URLSession.shared.dataTask(with: url) { [weak self] (data, response, error) in
            guard let data = data else {
                return
            }
            guard let image = UIImage(data: data) else {
                return
            }
            ImageCache.setObject(image, forKey: url as NSURL)
            
            guard let self = self else {
                return
            }
            guard let url = response?.url else {
                return
            }
            DispatchQueue.main.async {
                if self.imageURL == url {
                    self.image = image
                }
            }
        }
        task.resume()
        self.task = task
    }
    
    public private(set) var imageURL: URL? {
        get {
            let box: Box<URL>? = getAssociatedObject(self, &imageURLKey)
            return box?.value
        }
        set {
            let box = newValue.map { Box($0) }
            setRetainedAssociatedObject(self, &imageURLKey, box)
        }
    }
    
    public private(set) var task: URLSessionTask? {
        get {
            let box: Box<URLSessionTask>? = getAssociatedObject(self, &imageTaskKey)
            return box?.value
        }
        set {
            let box = newValue.map { Box($0) }
            setRetainedAssociatedObject(self, &imageTaskKey, box)
        }
    }
    
}
