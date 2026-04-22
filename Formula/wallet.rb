class Wallet < Formula
  desc "OmsWallet C SDK"
  homepage "https://github.com/0xsequence/c-sdk"
  url "https://github.com/0xsequence/c-sdk/archive/refs/tags/v0.3.0.tar.gz"
  sha256 "e4949620a97b2952ec774280df2344c5a047d4e96a19c8019f96e9a0c365996c"
  license "MIT"

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "curl"
  depends_on "cjson"
  depends_on "secp256k1"

  def install
    system "cmake", ".", "-DCMAKE_INSTALL_PREFIX=#{prefix}", *std_cmake_args
    system "make", "install"
  end

  test do
    # Basic smoke test
    assert_match "Usage", shell_output("#{bin}/oms-wallet --help")
  end
end
