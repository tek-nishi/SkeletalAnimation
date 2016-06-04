#pragma once

//
// 雑多な処理
//

#include <string>

#if defined (_MSC_VER)
using u_int = unsigned int;
#endif


// ファイル名を返す
// ex) hoge/fuga/piyo.txt -> piyo.txt
std::string getFilename(const std::string& path) {
  return path.substr(path.rfind('/') + 1, path.length());
}
