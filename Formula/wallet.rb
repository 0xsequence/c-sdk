class Wallet < Formula
  desc "Sequence C SDK"
  homepage "https://github.com/0xsequence/c-sdk"
  url "https://github.com/0xsequence/c-sdk/archive/refs/tags/v0.2.0.tar.gz"
  sha256 "SHA256_OF_TARBALL"
  license "MIT"

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "mbedtls"
  depends_on "curl"
  depends_on "cjson"
  depends_on "libsecp256k1"

  def install
    system "cmake", ".", "-DCMAKE_INSTALL_PREFIX=#{prefix}", *std_cmake_args
    system "make", "install"
  end

  test do
    # Basic smoke test
    assert_match "Usage", shell_output("#{bin}/sequence --help")
  end
end