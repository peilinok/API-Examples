//
//  AudioSpectrum.swift
//  APIExample
//
//  Created by Arlin on 2022/4/15.
//  Copyright © 2022 Agora Corp. All rights reserved.
//

/// Audio Spectrum
/// This module show how to obverser and show audio spectrum data.
/// 1.Enable Audio Spectrum: agoraKit.enableAudioSpectrumMonitor(200)
/// 2.Register obesever: agoraKit.registerAudioSpectrumDelegate(self)
/// 3.Call back AgoraAudioSpectrumDelegate to get local and remote spectrum data
///
/// More detail: Todo

import AgoraRtcKit
import AGEVideoLayout

class AudioSpectrumViewController: BaseViewController {
    @IBOutlet weak var videoContainer: AGEVideoContainer!
    
    let localAudioView = Bundle.loadVideoView(type: .local, audioOnly: true)
    let remoteAudioView = Bundle.loadVideoView(type: .remote, audioOnly: true)
    var localSpectrumpLayer: CAShapeLayer!
    var remoteSpectrumpLayer: CAShapeLayer!
    
    var agoraKit: AgoraRtcEngineKit!
    
    // MARK: - LifeCycle
    override func viewDidLoad() {
        super.viewDidLoad()
        self.setupUI()
        
        guard let channelId = configs["channelName"] as? String else {return}
        
        agoraKit = AgoraRtcEngineKit.sharedEngine(withAppId: KeyCenter.AppId, delegate: self)
        agoraKit.setClientRole(.broadcaster)
                
        // Enable Audio Spectrum and register data observer
        agoraKit.enableAudioSpectrumMonitor(200)
        agoraKit.register(self)
        
        let result = agoraKit.joinChannel(byToken: KeyCenter.Token, channelId: channelId, info: nil, uid: 0)
        if result != 0 {
            /// Error code description: https://docs.agora.io/en/Interactive%20Broadcast/error_rtc
            self.showAlert(title: "Error", message: "Join channel failed with errorCode: \(result)")
        }
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        self.updateUI()
    }
    
    override func didMove(toParent parent: UIViewController?) {
        if parent == nil {
            agoraKit.disableAudioSpectrumMonitor()
            agoraKit.leaveChannel(nil)
        }
    }
    
    // MARK: - UI
    func setupSpectrum(values: [NSNumber], layer: CAShapeLayer) {
        let count = values.count
        let rectWidth = layer.frame.size.width
        let rectHeight = layer.frame.size.height;
        let averageWidth = rectWidth / CGFloat(count)
        let lineWidth = averageWidth * 0.8
        let lineGap = averageWidth - lineWidth
        var x = lineGap / 2.0
        
        let path = UIBezierPath()
        for i in 0..<count {
            var lineHeight =  rectHeight - CGFloat(abs(values[i].floatValue))
            lineHeight = lineHeight < rectHeight ? lineHeight : rectHeight
            let linePath = UIBezierPath(rect: CGRect(x: x, y: rectHeight - lineHeight, width: lineWidth, height: lineHeight))
            path.append(linePath)
            x += lineWidth + lineGap
        }
        
        layer.path = path.cgPath
    }
    
    func setupUI () {
        localAudioView.setPlaceholder(text: self.getAudioLabel(uid: 0, isLocal: true))
        remoteAudioView.setPlaceholder(text: self.getAudioLabel(uid: 0, isLocal: false))
        localAudioView.statsInfo = nil
        remoteAudioView.statsInfo = nil
        videoContainer.layoutStream(views: [localAudioView, remoteAudioView])
                
        localSpectrumpLayer = CAShapeLayer.init()
        localSpectrumpLayer.fillColor = UIColor.lightGray.cgColor
        self.view.layer.addSublayer(localSpectrumpLayer)
        
        remoteSpectrumpLayer = CAShapeLayer.init()
        remoteSpectrumpLayer.fillColor = UIColor.lightGray.cgColor
        self.view.layer.addSublayer(remoteSpectrumpLayer)
    }
    
    func updateUI () {
        let x = 5.0
        let y = videoContainer.frame.origin.y + videoContainer.frame.size.height + 20.0
        let width = UIScreen.main.bounds.width - x * 2
        let height = 160.0;
        
        localSpectrumpLayer.frame = CGRect(x: x, y: y, width: width, height: height)
        remoteSpectrumpLayer.frame = CGRect(x: x, y: y + height + 30.0, width: width, height: height)
        
        let localLabel = UILabel(frame: CGRect(x: x, y: y + height + 10.0, width: width, height: 20.0))
        localLabel.text = "Local Audio Spectrum".localized
        localLabel.textColor = UIColor.gray
        self.view.addSubview(localLabel)
        
        let remoteLabel = UILabel(frame: CGRect(x: x, y: remoteSpectrumpLayer.frame.origin.y + height + 10.0, width: width, height: 20.0))
        remoteLabel.text = "Remote Audio Spectrum".localized
        remoteLabel.textColor = UIColor.gray
        self.view.addSubview(remoteLabel)
    }
}

// MARK: - AgoraAudioSpectrumDelegate
extension AudioSpectrumViewController: AgoraAudioSpectrumDelegate {
    func onLocalAudioSpectrum(_ audioSpectrumData: [NSNumber]?) -> Bool {
        DispatchQueue.main.async {
            self.setupSpectrum(values: audioSpectrumData!, layer: self.localSpectrumpLayer)
        }
        return true
    }
    
    func onRemoteAudioSpectrum(_ AudioSpectrumInfo: [AgoraAudioSpectrumInfo]?) -> Bool {
        guard let audioSpectrumData = AudioSpectrumInfo?.first?.audioSpectrumData,
                  audioSpectrumData.count > 0 else {return true}
        DispatchQueue.main.async {
            self.setupSpectrum(values: audioSpectrumData, layer: self.remoteSpectrumpLayer)
        }
        return true
    }
}

// MARK: - AgoraRtcEngineDelegate
extension AudioSpectrumViewController: AgoraRtcEngineDelegate {
    func rtcEngine(_ engine: AgoraRtcEngineKit, didOccurError errorCode: AgoraErrorCode) {
        /// Error code description: https://docs.agora.io/en/Interactive%20Broadcast/error_rtc
        LogUtils.log(message: "Error occur: \(errorCode)", level: .error)
        self.showAlert(title: "Error", message: "Error: \(errorCode.description)")
    }
    
    func rtcEngine(_ engine: AgoraRtcEngineKit, didJoinChannel channel: String, withUid uid: UInt, elapsed: Int) {
        LogUtils.log(message: "Join \(channel) with uid \(uid) elapsed \(elapsed)ms", level: .info)
        localAudioView.setPlaceholder(text: self.getAudioLabel(uid: uid, isLocal: true))
    }
    
    func rtcEngine(_ engine: AgoraRtcEngineKit, didJoinedOfUid uid: UInt, elapsed: Int) {
        LogUtils.log(message: "Remote user \(uid) joined elapsed \(elapsed)ms", level: .info)
        remoteAudioView.setPlaceholder(text: self.getAudioLabel(uid: uid, isLocal: false))
    }
    
    func rtcEngine(_ engine: AgoraRtcEngineKit, didOfflineOfUid uid: UInt, reason: AgoraUserOfflineReason) {
        LogUtils.log(message: "Remote user \(uid) offline with reason \(reason)", level: .info)
        remoteAudioView.setPlaceholder(text: self.getAudioLabel(uid: 0, isLocal: false))
    }
}

// MARK: - Entry ViewController
class AudioSpectrumEntryViewController: UIViewController {
    @IBOutlet weak var channelTextField: UITextField!

    @IBAction func joinBtnClicked(sender: UIButton) {
        guard let channelName = channelTextField.text,
              channelName.lengthOfBytes(using: .utf8) > 0 else {return}
        channelTextField.resignFirstResponder()
        
        let identifier = "AudioSpectrum"
        let storyBoard: UIStoryboard = UIStoryboard(name: identifier, bundle: nil)
        guard let newViewController = storyBoard.instantiateViewController(withIdentifier: identifier) as? BaseViewController else {return}
        newViewController.title = channelName
        newViewController.configs = ["channelName": channelName]
        self.navigationController?.pushViewController(newViewController, animated: true)
    }
}
