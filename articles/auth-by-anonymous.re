={auth-by-anonymous} 匿名認証でAuthenticationを利用する

//lead{
Firebase Authenticationでは匿名認証という形で、一時的なユーザーを発行できます。

本章ではその匿名認証の実装方法を解説します。
//}

== 匿名認証の特徴

匿名認証はGoogleやそのほかのアカウント、Eメールなどを使わずにその場でFirebaseUserを作成する認証方式です。
ユーザーに何の操作も必要としない反面、一度サインアウト状態になると二度とサインインすることはできないというデメリットがあります。
ただし、後述するアカウント連携を行うことで、再びサインイン可能なアカウントに切り替えることができます。

== コンソールから匿名認証を有効化する

GoogleアカウントやTwitterアカウント認証と同様に、コンソールのログイン方法から「匿名」を有効化します。

//image[enable_auth_anonymous][コンソールで認証ログインを有効化する][scale=0.65]{
//}

== コード実装

実装はとても簡単です。
@<code>{FirebaseAuth.signInAnonymously()} を呼び出すだけで完了です。

//list[call-signInAnonymously][AuthAnonymousViewModel.kt][kotlin]{
private lateinit var auth: FirebaseAuth

fun onSignIn() {
    auth.signInAnonymously()
        .addOnCompleteListener {
            // it.isSuccessful で成否が確認可能
        }
}
//}

//image[create_auth_anonymous][作成された匿名ユーザー（最下段）][scale=0.65]{
//}

匿名ユーザーは識別子に「（匿名）」が付くのが特徴です。
また、 @<code>{signInAnonymously()} を行うたびに、現在のサインイン状況に関係なく、新たにFirebase Userが発行されてサインインし直されてしまいます。

そのため実運用の際には、すでにユーザーログイン済みであるかどうかをチェックしてから匿名認証を行うようにしましょう。

//image[create_auth_anonymous2][サインインするたびにユーザーが増えます][scale=0.65]{
//}

== 別認証と連携し、再びサインイン可能にする

一時的に発行された匿名ユーザーは、ほかの認証方式とリンクさせることで、再びサインイン可能なユーザーへ更新できます。
今回は、これまでに解説したGoogleアカウント認証方式とTwitterアカウント認証方式それぞれのリンク方法を紹介します。

=== Googleアカウント認証と匿名ユーザーをリンクする

Googleアカウント認証と連携を行うためには、通常のGoogle認証の途中で取得するクレデンシャル情報が必要です。
Google認証の処理は『GoogleアカウントでAuthenticationを利用する』を参照してください。
@<code>{onActivityResult()} で返却されたIntent情報を受け取って、ViewModelに渡すまでは同じです。

次に掲載するコードは、ViewModelでIntentを受け取ってからの処理です。

//list[link-googleCredential][AuthAnonymousViewModel.kt][kotlin]{
fun onCallbackUri(intent: Intent) {
    val task = GoogleSignIn.getSignedInAccountFromIntent(intent)
    try {
        task.getResult(ApiException::class.java)?.let { account ->
            val credential = 
                GoogleAuthProvider.getCredential(account.idToken, null)
            auth.currentUser?.run {
                linkWithCredential(credential)
                    .addOnCompleteListener {
                        // it.successfulでリンク成否を確認可能
                    }
            }
        }
    } catch (exception: ApiException) {
    }
}
//}

@<code>{GoogleAuthProvider.getCredential()} を使ってcredential情報を受け取り、FirebaseAuthではなくFirebaseUserにcredentialを渡す形でリンク処理を行います。
コールバックの動きにほとんど違いはありません。
これだけでGoogle認証との連携が完了し、以降はリンクしたGoogleアカウントからの認証でサインインができます。

//image[link_anonymous1][匿名認証で作成したユーザー][scale=0.65]{
//}

//image[link_anonymous2][Google認証と連携することで、識別子とプロバイダが変化した][scale=0.65]{
//}

=== Twitterアカウント認証（OAuthProvider）と匿名ユーザーを連携する

Googleアカウト認証ではクレデンシャル情報を渡すことでリンク処理を行いました。
ではTwitterアカウント認証との連携はどうやるのでしょうか。
『TwitterアカウントでAuthenticationを利用する』で解説していますが、OAuthProviderは @<code>{startActivityForSignInWithProvider()} を呼び出した後はすべてOAuthProviderが処理してくれるため、クレデンシャル情報を取り出すタイミングがありません。

答えを書くと、FirebaseUserに @<code>{startActivityForLinkWithProvider()} が用意されているため、こちらを代わりに呼び出せば、同様にリンク処理も含めてOAuthProvider側で行ってくれます。

//list[link-twitterAccount][AuthAnonymousActivity.kt][kotlin]{
link2_button.setOnClickListener {
    val provider = OAuthProvider.newBuilder("twitter.com")
        .addCustomParameter("language", "ja")
        .build()
    viewModel.getAuth().currentUser?.run{
        startActivityForLinkWithProvider(this@AuthAnonymousActivity, provider)
            .addOnSuccessListener {
                Timber.d("success")
            }
            .addOnFailureListener {
                Timber.e(it)
            }
    }
}
//}

基本的に、ゼロからサインインする場合はFirebaseAuth、すでにあるユーザーにほかの認証の連携を行いたい場合はFirebaseUserに対して処理を呼び出す作りになっているようです。

=== 連携するアカウントの注意点

匿名認証と連携するアカウントは、まだFirebaseAuthでサインインされたことのないアカウントである必要があります。
すでにFirebaseUserが作成されたアカウントでは連携が失敗します。

また、この連携は匿名アカウントだけでなく、たとえばGoogleアカウントで作成したFirebaseUserにTwitterアカウント認証を連携することも可能です。

カスタム認証との連携に関しては未調査なので不明です。
ただ可能だとしても、ユーザーを作成せずにcredentialを発行して返却するサーバ側の処理が別途必要になるため、実装の難易度はかなり高いものと思われます。
そういった理由で、team Y.G.E.では採用することはないでしょう。