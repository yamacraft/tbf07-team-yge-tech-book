={what-is-firebase-authentication} Firebase Authenticationとは何か

//lead{
本章では、Firebase Authenticationについての簡単な概要を紹介します。
//}

== Firebase Authenticationとは何か

Firebase Authenticationとは、Firebaseが提供するユーザー認証機能です。
アカウントのデータ管理をFirebase側で行ってくれるので、こちら側でデータベースを用意する必要がありません。
さらにデータの管理部分もデベロッパーからは不可侵となるレベルで切り離されて管理されているので、下手に自分たちでゼロから認証システムを作るより、よっぽどセキュアです。@<fn>{what-auth-security-data}

さらに認証方式も豊富にそろっています。

 * Eメール＋パスワード認証
 * フェデレーションIDプロバイダ認証
 ** Googleアカウント認証（Google ゲームアカウント認証含）
 ** Facebookアカウント認証
 ** Twitterアカウント認証
 ** GitHubアカウント認証
 ** Microsoftアカウント認証
 ** Yahoo!アカウント認証@<fn>{what-auth-account-yahoo}
 ** Apple GameCenter認証@<fn>{note-auth-game-center}
 * 電話番号認証
 * 匿名認証

他にもFirebase Admin SDKを利用したカスタム認証システムを使うことで、上記以外の形でもユーザー認証を実装できます。

今回はteam Y.G.E.のプロダクト開発で利用を検討している、以下の認証方式での実装方法を解説します。

 * Googleアカウント認証
 * Twitterアカウント認証
 * カスタム認証（Twitchアカウント認証）
 * 匿名認証

//footnote[what-auth-security-data][UIDの確認やユーザーデータの削除や凍結はコンソール上から操作できますが、実際にどのようしてデータが内部で管理されているかをデベロッパーが知ることはできません。]

//footnote[what-auth-account-yahoo][Yahoo! JAPANではなく、.comの方のYahoo!です。]

//footnote[note-auth-game-center][2019年9月現在、ベータ提供。]

=== 個人的な認証方法の採択について

あくまでも個人的な意見ですが、できればFirebase Authenticationの認証は、フェデレーションIDプロバイダ認証のみを採用したいと考えています。

その理由として、まだFirebase Authenticationには満足な二段階認証関係のセキュリティが実装されていないためです。
某モバイル決済サービスの記者会見で、二段階認証が何かを知らない受け答えをして話題になった出来事がありました。
そこで多くのITエンジニアが衝撃を受けていましたが、そんなご時世で二段階認証が設定できないサービスを運用するのはいろんな方面でハイリスクです。

そういった理由で、Firebase Authenticationのみで管理されるEメール＋パスワード認証とSMS認証を採用する理由はありません。
真っ先に選択肢から切り捨てます。
できればフェデレーションIDプロバイダに対応されているアカウントで、二段階認証が設定できるものを採択するのが望ましいなと思います。@<fn>{twofa-provider-account}

//footnote[twofa-provider-account][ちゃんと調べたわけではありませんが、おそらく対応するアカウントすべてが二段階認証に対応しているようです。]

== Firebase Authenticationで登録できる情報

Firebase Authenticationで作成したユーザー（FirebaseUser）には、一意情報であるUIDのほかに、最低限度のユーザー情報が合わせて保存できるようになっています。

 * 表示名
 * Eメールアドレス
 ** メールアドレス検証の有無
 * 電話番号
 * プロフィール画像用URL

電話番号とEメールアドレスは、コンソール@<fn>{link-console}上からユーザーを特定するための条件として利用ができます。
プロフィール画像は画像データそのものを格納できるわけではないので、必要に応じて画像データの保存先を用意する必要があります。

//image[console-authentication][コンソール画面][scale=0.8]{
//}

//footnote[link-console][https://console.firebase.google.com/]

===[column] Firebase Authenticationのコンソールに関する注意点

公式が提供してくれているコンソールは、最小限の機能しか用意されていません。
たとえば作成したユーザーの数や、ユーザーの表示名やプロフィール画像の確認等を行うことはできません。
特定のユーザー検索も、UIDか電話番号かメールアドレスのいずれかでしか検索できないようになっています。

ユーザー管理を本格的に行いたい場合は、Firebase Admin SDKを使って別途管理ツールを用意する必要があります。
本書ではそこまで触れませんのでご注意ください。

===[/column]

== Firebaseの各機能とAuthenticationの関わり

FirestoreやRealtimeDatabase、Storageの読み書きルールには、Firebase Authenticationの認証状況を条件に使うことができます。
そのため、これらの機能を活用したい場合はAuthenticationによる認証の実装が避けられないものだと思います。

//list[rule-of-write-auth][書き込みのみ認証を必須とするFirestoreルール][txt]{
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    match /{document=**} {
      allow read: if true;
      allow write: if request.auth.uid != null;
    }
  }
}
//}

これらの機能で認証を必要とする場合、それぞれの機能で何か認証周りの実装を行う必要はありません。
裏側ですべてよい感じに対応してくれるので、デベロッパーは気にせず各機能の実装に集中できます。

本書ではこの部分に詳しく触れませんが、サンプルコードの動作元であるプロジェクトでは、検証として、書き込みは認証必須としたFirestoreにサインイン時間を書き込む処理を実装しています。

//list[set-on-last-signin-firestore][AuthViewModel.kt][kotlin]{
class AuthViewModel : ViewModel() {
    private lateinit var auth: FirebaseAuth
    private lateinit var db: FirebaseFirestore

    // Activity.onCreate()で呼び出す
    fun onCreated() {
        auth = FirebaseAuth.getInstance()
        db = FirebaseFirestore.getInstance()
    }

    private fun putAuthUserData() {
        auth.currentUser?.let { user ->
            val userData = AuthUserData(userName, Date().time)
            db.collection("lastLogin")
                .document(user.uid)
                .set(userData)
                .addOnSuccessListener {
                    // 書き込み成功
                }
                .addOnFailureListener {
                    // 書き込み失敗
                }
        }
    }
}
//}

任意のタイミングで @<code>{putAuthUserData()} を呼び出すことで、lastLoginコレクション内にuidでドキュメント化した「最終ログイン情報＋ユーザーの表示名」を書き込みます。

このコードからもわかるように、FirebaseAuthとFirebaseFirestoreを直接つなげている処理はありません。
FirebaseAuth側で認証処理をやっておけば、必要な認証周りの連携はすべてSDK内でやってくれます。

===[column] Firebase UIについて

本書の始まりは、カスタム認証システムの実装に関する検証からでした。
そのため、あらゆる実装がFirebase AuthenticationのAPIをそのまま使う形になっています。

しかし認証部分に絞ると、Googleが公式でFirebase UIというUIライブラリを用意しています。@<fn>{link-firebase-ui-document}
UIライブラリというだけあって、デザイン面の改修が（ゼロからやるのに比べて）面倒くさいという問題に目をつぶれば、複数の認証方式やアカウント連携が簡単にできるという利点があります。

さらにスマートロックにも対応していたりする反面、１年以上前にサポートが停止したTwitter Kitを利用していたり、カスタム認証に対応していないといった懸念点があります。

===[/column] 

//footnote[link-firebase-ui-document][https://firebase.google.com/docs/auth/android/firebaseui]