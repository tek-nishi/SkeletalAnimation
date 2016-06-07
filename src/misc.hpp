#pragma once

//
// 雑多な処理
//

#include <string>

#if defined (_MSC_VER)
using u_int = unsigned int;

// UTF-8 → cp932
std::string UTF8toCP932(const std::string& srcUTF8) {
  // Unicodeへ変換後の文字列長を得る
  int lengthUnicode = MultiByteToWideChar(CP_UTF8, 0, srcUTF8.c_str(), srcUTF8.size(), nullptr, 0);

  // UTF8からUnicodeへ変換
  std::vector<wchar_t> bufUnicode(lengthUnicode);
  MultiByteToWideChar(CP_UTF8, 0, srcUTF8.c_str(), srcUTF8.size(), &bufUnicode[0], lengthUnicode);

  // CP932へ変換後の文字列長を得る
  int lengthCP932 = WideCharToMultiByte(CP_THREAD_ACP, 0, &bufUnicode[0], lengthUnicode, nullptr, 0, nullptr, nullptr);

  // UnicodeからShiftJISへ変換
  std::vector<char> bufCP932(lengthCP932);
  WideCharToMultiByte(CP_THREAD_ACP, 0, &bufUnicode[0], lengthUnicode, &bufCP932[0], lengthCP932, nullptr, nullptr);

  std::string dstCP932(bufCP932.begin(), bufCP932.end());

  return dstCP932;
}

#endif


// FIXME:WindowsはVisualStudioでのパスの扱いがcp932。それを吸収するためのワークアラウンド
#if defined (_MSC_VER)
#define PATH_WORKAROUND(path) UTF8toCP932(path)
#else
#define PATH_WORKAROUND(path) (path)
#endif


// ファイル名を返す
// ex) hoge/fuga/piyo.txt -> piyo.txt
std::string getFilename(const std::string& path) {
  return path.substr(path.rfind('/') + 1, path.length());
}

// 切り上げて一番近い２のべき乗値を求める
int int2pow(const int value) {
	int res = 1;

	while (res < (1 << 30)) {
		if (res >= value) break;
		res *= 2;
	}

	return res;
}
