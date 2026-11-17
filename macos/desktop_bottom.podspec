#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint desktop_bottom.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'desktop_bottom'
  s.version          = '0.1.1'
  s.summary          = 'macOS desktop-bottom window mode for Flutter.'
  s.description      = <<-DESC
Windows/Linux/macOS desktop-bottom window mode for Flutter.
                       DESC
  s.homepage         = 'https://github.com/SQSora/desktop_bottom'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'desktop_bottom contributors' => 'noreply@example.com' }

  s.source           = { :path => '.' }
  s.source_files = 'desktop_bottom/Sources/desktop_bottom/**/*'

  s.dependency 'FlutterMacOS'

  s.platform = :osx, '10.15'
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
  s.swift_version = '5.0'
end
