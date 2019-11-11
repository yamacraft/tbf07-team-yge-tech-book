={predef} はじめに

== 本書について

本書はteam Y.G.E.内におけるプロダクト開発で採用を検討している技術の実装方法や検証結果、考察等をまとめた本になります。
主な対象者はteam Y.G.E.自身です。
そのため、「自分たちには不要だな」と判断した部分は、本書では触れていません。

今回は本格的にユーザー認証を取り入れたプロダクトを開発したいと考えていたので、Firebase Authenticationを使った認証周りの内容となりました。

== 本書の取り扱いについて（簡易版）

本書の紙版は、技術書典７のみでの頒布となります@<fn>{note-distribution}。

その後、本書は9月下旬（技術書典７の１週間後を目安）にboothで電子版の頒布を開始予定です。
さらに10月末から11月初頭を目処に、本書のRe:VIEWプロジェクトをGitHub上にて公開します。
詳細は後書きの『本書の今後の取り扱い』をご参照ください。

//footnote[note-distribution][12月に開催される「技術書同人誌博覧会」への参加を検討しており、こちらに参加した場合は後述のプロジェクト公開に関係なく、本書の紙版の頒布を予定しています。]

===[column] 『Android Thingsで作るバーチャルアバターシステム制作記録』について

技術書典６で頒布した『Android Thingsで作るバーチャルアバターシステム制作記録』の電子版販売やRe:VIEW公開がいまだに行われていませんが、これはAndroid Things自体が終了してしまったことによる、大幅なモチベーション低下が原因となっています。
お許しください。

===[/column]

== 本書のサンプルコードについて

冒頭にも書いたように、本書はteam Y.G.E.のプロダクトでの利用を前提としています。
そのため、コードはteam Y.G.E.都合で以下を前提として書いています。

 * Android Studioやライブラリは、基本的にStableの最新版を使う
 * API通信はRetrofit2を利用する
 * 非同期処理はCoroutineを利用する
 * WVVM@<fn>{note-guide-samplecode-wvvm}で書く

そのほか、イレギュラーな部分があれば章ごとに記述しています。

//footnote[note-guide-samplecode-wvvm][付録の『WVVMについて』を参照ください]

== team Y.G.E.とは

プライベートのプロダクト開発チームです。
といってもメンバーは実質筆者ひとりです。

最近は表立った活動がほぼ止まっていますが、年内に新規プロダクト開発を目指して動き始めています。
開発しているプロダクトなどは、公式サイト@<fn>{link-team-yge}をご覧ください。

//footnote[link-team-yge][https://yge.yamaglo.jp/]

== 免責事項

本書に記載した内容は、情報の提供のみを目的としています。
本書の内容を採用したことによる開発や、運用で起こった結果について、筆者はいかなる責任も持ちません。
読者の判断によって行ったものとして、各自で責任をお持ちください。