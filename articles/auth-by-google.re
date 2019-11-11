={auth-by-google} GoogleアカウントでAuthenticationを利用する

//lead{
本章から、実際に認証処理を実装するための方法を解説します。

本章ではGoogleアカウントによる認証方法について解説します。
//}

== コンソールからGoogleアカウントログインを有効化する

GoogleアカウントでFirebase認証を行うためには、まず対象のFirebaseプロジェクトでGoogleアカウントの利用を有効にする必要があります。

Firebaseのコンソール@<fn>{url-console-firebase}にアクセスし、「Authentication」の「ログイン方法」から、ログインプロバイダにあるGoogleのスイッチを「有効」にセットしましょう。

//image[enable_google_auth][Googleアカウントからのログインを有効化する][scale=0.65]{
//}

有効化する際に、サポートメールアドレスの設定が必要になります。
これはFirebaseプロジェクトに登録されたメンバーのメールアドレスを選択するようになっています。

また、実装には有効にした後に表示される「ウェブクライアントID」が必要になります。
「ウェブクライアント」とありますが、クライアント全般のようです。

//image[check_google_auth_web_client_id][ログイン有効後の画面][scale=0.65]{
//}

これで、コード実装前の準備は完了です。

//footnote[url-console-firebase][https://console.firebase.google.com/]

== Googleアカウント認証を実装する

Googleアカウント認証は、以下の流れで認証が行われます。

 * Googleアカウント認証用のIntentを作成して呼び出す
 * ユーザーがサインインに使うGoogleアカウントを選択する
 * @<code>{onActivityResult()} で選ばれたGoogleアカウント情報が返ってくるので、その中からクレデンシャル情報を抜き出してAuthenticationに渡す
 * FirebaseUserのサインイン（必要なら作成も含む）が完了する

Intent作成以外の部分はContextを必要しない部分ですので、それ以外はすべてViewModel内で実装することにします。

=== 必要なライブラリを追加

まず手始めに、Googleアカウントの認証を行うためのライブラリをプロジェクトに追加します。

//list[add_dependencies_google_auth][app/build.gradle][gradle]{
// この2つは必須
implementation 'com.google.firebase:firebase-core:17.2.0'
implementation 'com.google.firebase:firebase-auth:19.0.0'

// 追加
implementation 'com.google.android.gms:play-services-auth:17.0.0'
//}

=== Googleアカウント認証用のIntentを作成して呼び出す

Googleアカウントの作成や選択といった部分も含めたOAuth認証周りの部分、すべてAndroidのOS（というよりGoogle Play Services？）側にIntentを投げてお任せすることになります。
実装する側が、ユーザーがどのようにして認証に使うGoogleアカウントを選択したかを気にする必要はありません。

//list[call-google-signin-intent][AuthGoogleActivity.kt][kotlin]{
sign_in_button.setOnClickListener {
    val options = 
        GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
            .requestIdToken(/* ウェブクライアントID */)
            .build()
    val googleSignInClient = GoogleSignIn.getClient(this, options)

    val intent = googleSignInClient.signInIntent
    startActivityForResult(intent, RC_SIGN_IN)
}
//}

サインインのボタンをタップすることで、認証用のIntentを飛ばす処理になっています。

===[column] requestEmail()について

公式のドキュメントでは、GoogleSignInOptionsに @<code>{.requestEmail()} を追加していますが、これは必須ではありません。
後述しますが、メールアドレスをリクエストすることでFirebaseUserのメールアドレス情報にGoogleアカウントのメールアドレスが登録されるだけでなく、「識別子」という情報にもGoogleアカウントのメールアドレスが設定されます。
これによって、UIDだけでなくメールアドレスからもユーザーを特定できるようになるため運用面では便利なのですが、個人的には必要以上の個人情報を取得したくありません。
ですので、メールアドレスのリクエストはオプションから外すことにしました。

===[/column]

作成したIntentを @<code>{startActivityForResult()} に投げることで、ユーザーへ認証に使うアカウントを選択する画面が表示されます。

//image[selectable_google_account][呼び出される画面例（端末に複数のGoogleアカウントが登録されている場合）][scale=0.5]{
//}

@<code>{startActivityForResult()} でIntentを投げているので、ユーザーが認証に使うアカウントを選択することで、 @<code>{onActivityResult()} で選択された結果を受け取れます。

//list[google-signin-onactivityresult][AuthGoogleActivity.kt][kotlin]{
override fun onActivityResult(requestCode: Int, 
                              resultCode: Int,
                              data: Intent?) {
    super.onActivityResult(requestCode, resultCode, data)
    if (requestCode == RC_SIGN_IN
        && resultCode == Activity.RESULT_OK
        && data != null
    ) {
        viewModel.onCallbackUri(data)
    }
}
//}

選択結果はdata内にあります。
そのままViewModelに渡して、後はそちら側で処理することにします。

=== クレデンシャル情報を取り出してAuthenticationにサインインする

受け取ったIntent情報から、選択されたGoogleアカウントのクレデンシャル情報を取得することは難しくありません。
すべてAPIが用意されているので、順番通り正しく呼び出すだけです。

//list[signin-auth-by-google][AuthGoogleViewModel.kt][kotlin]{
private lateinit var auth: FirebaseAuth

fun onCallbackUri(intent: Intent) {
    val task = GoogleSignIn.getSignedInAccountFromIntent(intent)
    try {
        task.getResult(ApiException::class.java)?.let { account ->
            val credential = 
                GoogleAuthProvider.getCredential(account.idToken, null)
            auth.signInWithCredential(credential)
                .addOnCompleteListener { task ->
                    // task.isSuccessful で成否判定が可能
                }
        }
    } catch (exception: ApiException) {
    }
}
//}

まずIntentからFirebase Authenticationの認証に必要なクレデンシャル情報を取り出します。
取り出したクレデンシャル情報をそのまま @<code>{signInWithCredential()} に渡すことで、まだ未登録のGoogleアカウントであれば新しいFirebaseUserの作成を行い、すでに登録済みであれば既存のFirebaseUserへサインインする処理が行われます。どちらでも @<code>{task.isSuccessful} はtrueが返ります。

なお、新規作成の際はGoogleアカウントの表示名とプロフィール画像のURLが、FirebaseUserの表示名とプロフィールURL情報に登録されています。
今回は省いた　@<code>{.requestEmail()} を行っていれば、メールアドレスも検証済みの状態で追加されます。

以上でGoogleアカウントによるFirebase Authenticationの認証処理の実装は完了です。
非常に短く、かつActivityとViewModelで処理を単純に分断化して実装できました。
個人的には特に不都合な理由がなければ、積極的に採用したい認証方式という印象です。