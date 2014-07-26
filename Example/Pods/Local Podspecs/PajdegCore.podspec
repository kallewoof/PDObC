#
# Be sure to run `pod lib lint PajdegCore.podspec' to ensure this is a
# valid spec and remove all comments before submitting the spec.
#
# Any lines starting with a # are optional, but encouraged
#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html
#

Pod::Spec.new do |s|
  s.name             = "PajdegCore"
  s.version          = "0.0.3"
  s.summary          = "C library for mutating PDF files"
  s.description      = <<-DESC
  Pajdeg is a self-contained C library for mutating (modifying) PDFs. 
  
  It's built around the principles of a pipe, where the original PDF is examined, needed changes are determined and tasks are "attached" based on page number, object ID, etc. and then simply sent through a pipe into a new PDF with the desired modifications. 
                       DESC
  s.homepage         = "https://github.com/kallewoof/PajdegCore"
  # s.screenshots     = "www.example.com/screenshots_1", "www.example.com/screenshots_2"
  s.license          = 'MIT'
  s.author           = { "Karl-Johan Alm" => "kalle.alm@gmail.com" }
  s.source           = { :git => "https://github.com/kallewoof/PajdegCore.git", :tag => s.version.to_s, :submodules => true }
  s.social_media_url = 'https://twitter.com/kallewoof'

  #s.platform     = :ios, '7.0'
  s.requires_arc = false

  s.source_files = 'Pod/Source/src/'
  #s.resources = 'Pod/Assets/*.png'
  
  s.xcconfig = { 'OTHER_LDFLAGS' => '-l z.1' }

  s.public_header_files = 'Pod/Source/src/*.h'
  # s.frameworks = 'UIKit', 'MapKit'
  # s.dependency 'AFNetworking', '~> 2.3'
end
