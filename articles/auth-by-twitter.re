={auth-by-Twitter} OAuthProviderを使ってTwitterアカウントでAuthenticationを利用する

//lead{
本章ではOAuthProviderを使った認証の一例として、Twitterアカウントによる
認証方法について解説します。
//}

== OAuthProviderについて

TwitterやFacebook、GitHubなどのアカウントを使った認証処理は、いつごろからかOAuthProviderという汎用のプロバイダクラスを使って認証ができるようになりました。
具体的にどれだけのアカウントが対応されているかは、OAuthProvider自体のコードが公開されていないため確認ができません。
公式ドキュメントから判断するしかないようです。

今回はこのOAuthProviderを使った認証の一例として、Twitterアカウントからの認証を実装します。

=== Twitterのデベロッパー登録に関する諸注意

Twitterアカウントで認証を行うためには、後述するコンソールでの有効化のさらに前段階として、Twitterデベロッパーサイトでappsの登録が必要になります。

これまでは比較的簡単に登録できたTwitterのデベロッパー申請とapps登録でしたが、１年ほど前から完全審査制となってしまったため、手続きが複雑化し、とても面倒くさくなってしまいました。

今回紹介するのは、あくまでもFirebase Authenticationの実装です。
デベロッパー登録に関する説明をすると長くなりすぎるので本書では省略します。

//image[twitter_debeloper_apps][Twitterデベロッパーサイトのapps詳細][scale=0.65]{
//}

今回はTwitterアカウントの認証が行いたいだけですので、権限がREADだけでもよいので認証に使うappsをひとつ登録してください。
Consumer API keysのAPI keyとAPI secret Keyを利用することになります。

== コンソールからTwitterアカウントログインを有効化する

Googleアカウントのときと同様に、まず準備段階としてFirebaseのコンソールからTwitterログインを有効化する必要があります。

//image[enable-auth-by-twitter][コンソールにTwitterログインを追加][scale=0.65]{
//}

APIキーとAPIシークレットに、前節で触れたConsumer API keysのAPI keyとAPI secret Keyを登録します。
コールバック用のURLは今回の実装では利用しません。

ちなみに画像ではなんとなくモザイク化させていますが、コールバック用のURLは通常GETのパラメータとして使われる時点でURLで見えてしまうものですので、がんばって秘匿するほどのものではありません。

== OAuthProviderによるTwittwerアカウント認証を実装する

それでは必要な準備が終わりましたので、さっそく実装していきます。
OAuthProviderでは、以下の流れで認証を行います。

 * OAuthProviderを作成する
 * OAuthProviderのメソッドを利用して、FirebaseAuthから必要なintentを生成して必要なActivityの立ち上げまでを依頼する

これだけで完了です。
Twitterのサインイン結果の取得やクレデンシャル情報の取り出し、Firebase Authenticationへのサインイン処理もすべてOAuthProvider側で行ってくれます。

=== OAuthProviderを作成する

まずはOAuthProviderを作成します。
必要な情報をセットするとは書きましたが、Twitterに関してはほとんどセットする情報がありません。

//list[add-OAuthProvider][AuthTwitterActivity.kt][kotlin]{
val provider = OAuthProvider.newBuilder("twitter.com")
    .addCustomParameter("language", "ja")
//}

メインはどのプロバイダのアカウントを使うかという情報が必要なだけです。
ほかに必要なパラメータがあれば、 @<code>{addCustomParameter()} で追加していきます。

今回は言語情報だけ追加していますが、Twitterの場合CustomTabsでTwitterのサインインページを開くことになるので、表示言語はTwitter側で対応してくれるはずです。
特に必要なものではありません。

=== FirebaseAuthから認証処理を呼び出す

必要なOAuthProviderを作成したので、これをもとに @<code>{FirebaseAuth.startActivityForSignInWithProvider()} を呼び出します。

//list[create-startActivityForSignInWithProvider][AuthTwitterActivity.kt][kotlin]{
val provider = OAuthProvider.newBuilder("twitter.com")
    .addCustomParameter("language", "ja")

FirebaseAuth.getInstance()
    .startActivityForSignInWithProvider(this, provider.build())
    .addOnSuccessListener {
        // 認証成功
    }
    .addOnFailureListener {
        // 認証失敗
    }
//}

なんとこの呼び出しだけで、OAuthProviderによる認証処理は完了です。
OAuthProvider側で必要な画面を判断し、FirebaseAuthへのサインイン処理もまとめて行ってくれます。
とても便利ですね。

//image[oauth-twitter][CustomTabでTwitterのOAuth認証画面が開かれる][scale=0.35]{
//}

=== 個人的な懸念点（FirebaseAuthはどこから参照するか）

ただし、個人的にとても気になることがあります。
それはActivity側で、FirebaseAuthのインスタンスが必要になってしまうことです。

基本的にActivity（View）側はFirebaseUserの情報に関心があっても、FirebaseAuthの状態を知る必要がないはずです。
ですので、今回のプロジェクトではViewModel側にFirebaseAuthを持たせる形で実装しています。
FirebaseAuth自体はシングルトンで生成されるようですので、複数ヵ所からインスタンスを生成しても問題はなさそうなのですが、この呼び出しのためだけに、View側にFirebaseAuthを生成するのは、個人的な気持ち悪さがあります。

それだけでなく、 @<code>{startActivityForSignInWithProvider()} では引数に呼び出し元のActivityが必要になります。
ViewModelではActivityのContextを参照させるべきではないと、公式のドキュメントにも記載されています。@<fn>{document-aac-viewmodel}

そうした理由で、最低限必要な部分だけViewModelで作らせるという分割もできません。
個人的にはなんだかいまいちな感じですが、ViewModel側からFirebaseAuthを受け取って呼び出す実装にしました。

//list[fixed-auth-by-twitter][AuthTwitterActivity.kt][kotlin]{
sign_in_button.setOnClickListener {
    val provider = OAuthProvider.newBuilder("twitter.com")
        .addCustomParameter("language", "ja")
    viewModel.getAuth()
        .startActivityForSignInWithProvider(this, provider.build())
        .addOnSuccessListener {
            // 認証完了
        }
        .addOnFailureListener {
            // 認証失敗
        }
}
//}

最終的に、OAthProviderを利用したTwitter認証はこのような実装となりました。

//footnote[document-aac-viewmodel][https://developer.android.com/topic/libraries/architecture/viewmodel]

=== Twitterアカウント認証の注意点

実装がとても簡単なTwitterアカウント認証ですが、ひとつ注意点があります。
それは、FirebaseUserの情報内にTwitterのスクリーンネーム（@xxx）もTwitterのUIDも格納されないことです。
Googleアカウント認証ではメールアドレス取得のリクエストを追加することで識別子にメールアドレスが登録されていましたが、Twitterアカウントでは何も登録されません。

いちおう、 @<code>{addOnSuccessListener()} 内で受け取ったAuthResult内から取り出せるcredential内に、TwitterのUIDを含んでいるようなtokenを見つけることができますが、正規にそれを取得することはできません。

そのため、Twitterの情報を元にFirebaseUserを特定することは困難というか、ほぼ不可能です。
こうした点を踏まえた上で、採用を検討してください。
おそらく、メールアドレスが取得できないそのほかのアカウントでも同様の問題が発生すると思われます。