cask "sentinel" do
  version "1.0.0"
  sha256 :no_check

  url "https://github.com/sopwit/sentinel/releases/download/v#{version}/Sentinel-macOS-#{version}.dmg"
  name "Sentinel"
  desc "Local-first AI desktop assistant"
  homepage "https://sentinel.dev"

  livecheck do
    url "https://github.com/sopwit/sentinel/releases.atom"
    strategy :github_latest
  end

  auto_updates true
  depends_on macos: ">= :monterey"

  app "Sentinel.app"

  uninstall quit:      "dev.sentinel.Sentinel",
            script:    {
              executable: "#{appdir}/Sentinel.app/Contents/Resources/uninstall.sh",
              must_succeed: false,
            }

  zap trash: [
    "~/Library/Application Support/Sopwit/Sentinel",
    "~/Library/Application Support/Sopwit/Sentinel Desktop",
    "~/Library/Caches/dev.sentinel.Sentinel",
    "~/Library/Logs/Sentinel",
    "~/Library/Preferences/dev.sentinel.Sentinel.plist",
  ]
end
