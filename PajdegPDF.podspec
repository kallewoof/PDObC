#
# Be sure to run `pod lib lint Pajdeg.podspec' to ensure this is a
# valid spec and remove all comments before submitting the spec.
#
# Any lines starting with a # are optional, but encouraged
#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html
#

Pod::Spec.new do |s|
  s.name             = "PajdegPDF"
  s.version          = "0.2.2"
  s.summary          = "Objective-C library for mutating PDF files"
  s.description      = <<-DESC
  Pajdeg is a self-contained library for mutating PDFs. 
  
  It's built around the principles of a pipe, where the original PDF is examined, needed changes are determined and tasks are "attached" based on page number, object ID, etc. and then simply sent through a pipe into a new PDF with the desired modifications. 
  
  This is the Objective-C wrapper for Pajdeg, which includes a number of improvements over the C core library.
                       DESC
  s.homepage         = "https://github.com/kallewoof/PDObC"
  # s.screenshots     = "www.example.com/screenshots_1", "www.example.com/screenshots_2"
  s.license          = 'MIT'
  s.author           = { "Karl-Johan Alm" => "kalle.alm@gmail.com" }
  s.source           = { :git => "https://github.com/kallewoof/PDObC.git", :tag => s.version.to_s }
  s.social_media_url = 'https://twitter.com/kallewoof'

  s.ios.deployment_target = '6.0'
  s.osx.deployment_target = '10.7'
  # s.platform     = :ios, '6.0'
  s.requires_arc = true

  s.source_files = 'Pod/Classes'
  # s.resources = 'Pod/Assets/*.png'

  # s.public_header_files = 'Pod/Classes/**/*.h'
  # s.frameworks = 'UIKit', 'MapKit'
  s.dependency 'PajdegCore', '~> 0.1.0'
end
