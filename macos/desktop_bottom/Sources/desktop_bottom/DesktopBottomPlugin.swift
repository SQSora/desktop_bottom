import Cocoa
import CoreGraphics
import FlutterMacOS

public class DesktopBottomPlugin: NSObject, FlutterPlugin {
  private weak var registrar: FlutterPluginRegistrar?
  private var enabled = false
  private var desktopMode = false
  private var savedState: SavedWindowState?

  private struct SavedWindowState {
    let level: NSWindow.Level
    let collectionBehavior: NSWindow.CollectionBehavior
    let styleMask: NSWindow.StyleMask
    let frame: NSRect
    let titleVisibility: NSWindow.TitleVisibility
    let titlebarAppearsTransparent: Bool
    let isMovableByWindowBackground: Bool
    let ignoresMouseEvents: Bool
    let hasShadow: Bool
    let closeButtonHidden: Bool
    let miniaturizeButtonHidden: Bool
    let zoomButtonHidden: Bool
  }

  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(
      name: "dev.sora.desktop_bottom",
      binaryMessenger: registrar.messenger
    )
    let instance = DesktopBottomPlugin(registrar: registrar)
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  init(registrar: FlutterPluginRegistrar) {
    self.registrar = registrar
    super.init()
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    DispatchQueue.main.async {
      self.handleOnMain(call, result: result)
    }
  }

  private func handleOnMain(
    _ call: FlutterMethodCall,
    result: @escaping FlutterResult
  ) {
    switch call.method {
    case "setEnabled":
      guard let value = call.arguments as? Bool else {
        result(FlutterError(
          code: "bad_arguments",
          message: "setEnabled 需要一个布尔参数",
          details: nil
        ))
        return
      }
      result(setEnabled(value))
    case "isEnabled":
      result(enabled)
    case "isSupported":
      result(window != nil)
    case "setDesktopMode":
      guard let value = call.arguments as? Bool else {
        result(FlutterError(
          code: "bad_arguments",
          message: "setDesktopMode 需要一个布尔参数",
          details: nil
        ))
        return
      }
      result(setDesktopMode(value))
    case "isDesktopMode":
      result(desktopMode)
    default:
      result(FlutterMethodNotImplemented)
    }
  }

  private var window: NSWindow? {
    registrar?.view?.window
  }

  private func saveStateIfNeeded(_ window: NSWindow) {
    guard savedState == nil else { return }
    savedState = SavedWindowState(
      level: window.level,
      collectionBehavior: window.collectionBehavior,
      styleMask: window.styleMask,
      frame: window.frame,
      titleVisibility: window.titleVisibility,
      titlebarAppearsTransparent: window.titlebarAppearsTransparent,
      isMovableByWindowBackground: window.isMovableByWindowBackground,
      ignoresMouseEvents: window.ignoresMouseEvents,
      hasShadow: window.hasShadow,
      closeButtonHidden:
        window.standardWindowButton(.closeButton)?.isHidden ?? false,
      miniaturizeButtonHidden:
        window.standardWindowButton(.miniaturizeButton)?.isHidden ?? false,
      zoomButtonHidden:
        window.standardWindowButton(.zoomButton)?.isHidden ?? false
    )
  }

  private func restoreStateIfNeeded(
    _ window: NSWindow,
    restoreFrame: Bool = true
  ) {
    guard let state = savedState else { return }
    window.styleMask = state.styleMask
    window.titleVisibility = state.titleVisibility
    window.titlebarAppearsTransparent = state.titlebarAppearsTransparent
    window.isMovableByWindowBackground = state.isMovableByWindowBackground
    window.ignoresMouseEvents = state.ignoresMouseEvents
    window.hasShadow = state.hasShadow
    window.standardWindowButton(.closeButton)?.isHidden =
      state.closeButtonHidden
    window.standardWindowButton(.miniaturizeButton)?.isHidden =
      state.miniaturizeButtonHidden
    window.standardWindowButton(.zoomButton)?.isHidden = state.zoomButtonHidden
    window.level = state.level
    window.collectionBehavior = state.collectionBehavior
    if restoreFrame {
      window.setFrame(state.frame, display: true)
    }
    savedState = nil
  }

  private func setEnabled(_ enabled: Bool) -> Bool {
    guard let window = window else {
      self.enabled = false
      desktopMode = false
      return false
    }

    if desktopMode {
      _ = setDesktopMode(false)
    }
    if self.enabled == enabled {
      return true
    }

    if enabled {
      saveStateIfNeeded(window)
      // Below normal windows, above desktop icons: keeps the window
      // interactive while staying out of the normal stacking order.
      window.level = NSWindow.Level(
        rawValue: Int(CGWindowLevelForKey(.normalWindow)) - 1
      )
      window.collectionBehavior = window.collectionBehavior.union([
        .stationary,
        .canJoinAllSpaces,
      ])
      self.enabled = true
    } else {
      // Bottom mode only changes level/collection behavior. Restoring the
      // saved frame here would undo moves/resizes made while it was enabled.
      restoreStateIfNeeded(window, restoreFrame: false)
      self.enabled = false
    }
    return true
  }

  private func setDesktopMode(_ enabled: Bool) -> Bool {
    guard let window = window else {
      self.enabled = false
      desktopMode = false
      return false
    }

    if desktopMode == enabled {
      return true
    }

    if enabled {
      if window.styleMask.contains(.fullScreen) {
        return false
      }
      if window.isMiniaturized {
        window.deminiaturize(nil)
      }

      saveStateIfNeeded(window)
      // Above desktop icons, below normal windows. This is the interactive
      // desktop-widget level; `.stationary` keeps it visible on “show desktop”.
      window.level = NSWindow.Level(
        rawValue: Int(CGWindowLevelForKey(.desktopIconWindow)) + 1
      )
      window.collectionBehavior = [
        .stationary,
        .canJoinAllSpaces,
        .ignoresCycle,
      ]
      // A titled window with a hidden/transparent titlebar keeps key/main
      // capability; AppKit borderless windows cannot become key by default.
      let desktopFrame = window.frame
      window.styleMask = [.titled, .fullSizeContentView]
      window.titleVisibility = .hidden
      window.titlebarAppearsTransparent = true
      window.standardWindowButton(.closeButton)?.isHidden = true
      window.standardWindowButton(.miniaturizeButton)?.isHidden = true
      window.standardWindowButton(.zoomButton)?.isHidden = true
      window.isMovableByWindowBackground = true
      window.ignoresMouseEvents = false
      window.hasShadow = false
      // Changing styleMask subtracts the titlebar height from the frame;
      // restore it so the widget keeps its previous size.
      window.setFrame(desktopFrame, display: true)
      window.makeKeyAndOrderFront(nil)
      desktopMode = true
      self.enabled = true
      return true
    }

    restoreStateIfNeeded(window)
    desktopMode = false
    self.enabled = false
    window.makeKeyAndOrderFront(nil)
    return true
  }
}
