={wvvm-overview} WVVMについて

//lead{
本書で掲載しているサンプルコード、並びにその元となっているサンプルプロジェクト「SampRa-android」は、WVVMというソフトウェアーキテクチャ兼思想にもとづいて作られています。

特に難しい作りのものではありませんが、いちおうどのような考えのものかを本章で解説します。
//}

== WVVMとは

WVVMはWhatever、View、ViewModelの3つで構成するソフトウェアアーキテクチャでありコーディング思想でもある、筆者の造語です。
AngularJSのMVWをリスペクトしたものになります。

Androidは幸いにして、Android Architecture ComponentsとAndroid Jetpackの登場によって、Viewと「ひとつ上の層」の切り分けが容易に行えるようになりました。
Viewは自身が表示するデータがどのようにして生成や更新がされるかを知る必要がなくなり、ViewModelはデータがどのように表示されるか、Viewがどのような仕様で動くかを考慮せずに作ることができます。

//image[explain-sample-wvvm][WVVMの構成図][scale=1.00]{
//}

WVVMではこの切り分けだけはしっかりと行い、ViewModelから先は既存のソフトウェアアーキテクチャパターンの定義にとらわれず、「現場の都合」を最優先に、自分たちの都合のよいように組み立てよう、あるべき姿に悩む前に、まずは手を動かして作ってみようという、ソフトウェアアーキテクチャパターンのようでそうではない、思想的なものです。

=== MVVMのView-ViewModelとWVVMのView-ViewModelとの違い

WVVM、というよりAndroid JetpackのViewModelは、ViewがViewModelの持つデータを監視してアクションを起こせますが、逆にViewModelがViewのデータを監視するしくみはありません。
もしかしたら何かあるかもしれませんが、公式のViewModelの概要ページ@<fn>{link-aac-viewmodel}に触れられていないので、推奨する使い方ではないのでしょう。

これだとMVVMの概要図であるようなViewとViewModelの相互監視とは言いにくい関係性になります。
ただ個人的にはこちらの方が好みです。

ついでに書くと、WVVMのアーキテクチャはだいたいMVVMなのですが、こうした部分などでMVVMと言い切ると余計な議論になりそうですので、そういった面倒ごとからの回避も兼ねてWVVMと呼ぶことにしています。
WVVMは、不本意な議論や思考から逃げるための自衛手段的思想でもあります。

//footnote[link-aac-viewmodel][https://developer.android.com/topic/libraries/architecture/viewmodel.html]

== Firebase AuthenticationとWVVM

いわゆる「関心の分離」にのっとって考えると、Viewはユーザーに認証結果やサインインしたユーザー情報を伝えることが必要であって、
「どうやって認証処理のやりとりをするか」
を知る必要はありません。@<fn>{note-sample-auth-view-viewmodel}

そのため、今回のサンプルプロジェクトでは基本的な処理の大半をViewModel内で行い、View側はFirebaseUserクラスを監視だけを行います。

//image[explain-sample-firebase-auth][今回の認証処理に関する役割分担][scale=1.00]{
//}

//footnote[note-sample-auth-view-viewmodel][ユーザーが操作を必要とする部分は除く]
