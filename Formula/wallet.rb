class Wallet < Formula
  desc "Sequence C SDK"
  homepage "https://github.com/0xsequence/c-sdk"
  url "https://github.com/0xsequence/c-sdk/archive/refs/tags/v0.2.2.tar.gz"
  sha256 "d891458dba853c3fb52d5170bb07453556adb1583df1587fcd900b46e3f729c2"
  license "MIT"

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "mbedtls"
  depends_on "curl"
  depends_on "cjson"
  depends_on "secp256k1"

  def install
    system "cmake", ".", "-DCMAKE_INSTALL_PREFIX=#{prefix}", *std_cmake_args
    system "make", "install"
  end

  test do
    # Basic smoke test
    assert_match "Usage", shell_output("#{bin}/sequence --help")
  end
end