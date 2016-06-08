# Assimp Loader Sample with Cinder 0.8.6
Assimpで読み込んだモデルデータ(アニメーション含む)をCinder 0.8.6で表示するサンプルです。

自分の学習向けなので内容は雑です。

以下のサンプルを含んでいます。

+ Assimpでモデルデータを読み込む
+ メッシュデータ
+ マテリアル
+ 階層アニメーション
+ スケルタルアニメーション
+ ダイアログによる簡易プレビューワ設定変更
+ iOS対応


## How to build
Cinderはもとより、Assimpのインクルードファイルとライブラリファイルをプロジェクトから参照できるようにしておいてください。

あと、テストデータは自前で用意してください。

## Attention
+ Windows環境でテクスチャのファイル名に２バイト文字が含まれている場合、Assimpの当該インポーターのパス変換処理に手を入れる必要があります。
+ ダイアログ(cinder::params)の実装にVisualStudio2010向けのワークアラウンドが含まれています。これを外してCinderライブラリを再ビルドしてください。


## Liense
License All source code files are licensed under the MPLv2.0 license

[MPLv2.0](https://www.mozilla.org/MPL/2.0/)