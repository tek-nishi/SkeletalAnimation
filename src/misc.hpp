#pragma once

//
// 雑多な処理
//

#include <string>

#if defined (_MSC_VER)
using u_int = unsigned int;

// UTF-8→cp932 変換
std::string UTF8toSjis(std::string srcUTF8) {
	//Unicodeへ変換後の文字列長を得る
	int lenghtUnicode = MultiByteToWideChar(CP_UTF8, 0, srcUTF8.c_str(), srcUTF8.size() + 1, NULL, 0);

	//必要な分だけUnicode文字列のバッファを確保
	wchar_t* bufUnicode = new wchar_t[lenghtUnicode];

	//UTF8からUnicodeへ変換
	MultiByteToWideChar(CP_UTF8, 0, srcUTF8.c_str(), srcUTF8.size() + 1, bufUnicode, lenghtUnicode);

	//ShiftJISへ変換後の文字列長を得る
	int lengthSJis = WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, -1, NULL, 0, NULL, NULL);

	//必要な分だけShiftJIS文字列のバッファを確保
	char* bufShiftJis = new char[lengthSJis];

	//UnicodeからShiftJISへ変換
	WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, lenghtUnicode + 1, bufShiftJis, lengthSJis, NULL, NULL);

	std::string strSJis(bufShiftJis);

	delete bufUnicode;
	delete bufShiftJis;

	return strSJis;
}

#endif


// FIXME:Windowsはパスの扱いがcp932。それを吸収するためのワークアラウンド
#if defined (_MSC_VER)
#define PATH_WORKAROUND(path) UTF8toSjis(path)
#else
#define PATH_WORKAROUND(path) (path)
#endif


// ファイル名を返す
// ex) hoge/fuga/piyo.txt -> piyo.txt
std::string getFilename(const std::string& path) {
  return path.substr(path.rfind('/') + 1, path.length());
}
