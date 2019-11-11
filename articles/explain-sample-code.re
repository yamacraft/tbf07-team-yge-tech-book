={explain-sample-code} サンプルコードについて

//lead{
本章では、本書に記載されているサンプルコード並びにサンプルプロジェクトについて簡単に解説します。
//}

== サンプルプロジェクト「SampRa-android」の紹介

本書に掲載しているサンプルコードは、GitHubで公開している「SampRa-android」というAndroidプロジェクト内で実際に動かしたコードから、解説部分に必要なものだけを抜粋して掲載しています。

//image[qr_sampra][https://github.com/yamacraft/SampRa-android][scale=2.00]{
//}

こちらのプロジェクトをビルドすることで、実際の動作が確認できます。

ただし、service-account.jsonや機密情報としている部分は.gitignoreでリポジトリから外したり、local.properties内で管理するようにしているため、落として即ビルドできるようにはなっていません。
当然、Firebaseプロジェクトも別途必要になります。
READMEを参考に、必要なファイル等を追加するなどしてください。

また、今回この機密部分の実装内容は解説しませんので、実際にプロジェクトの該当部分を参照いただければと思います。

== SampRa-android内での本書の該当部分について

本書の頒布時点ではおそらく本書に関する部分のコードしかありませんが、ViewとViewModel関連はui.authパッケージ以下に置かれています。

簡単な動きを解説すると、AuthActivity内ではFirebase Authenticationの状況の監視とサインアウト、一部ユーザー情報のアップデートだけを担っています。
各認証方式によるサインインは、ui.auth.signin以下のActivityを呼び出して、そこで行うようにしています。

Firebase Authenticationのステータス変化は @<code>{FirebaseAuth.onAuthStateChanged} リスナで監視できます。
AuthViewModelではAuthActivityが立ち上がってから破棄されるまでFirebaseAuthの状態を監視するようにして、変化があればFireStoreへサインイン時間を更新する処理を呼び出しています。

//list[call-putLastSignIn][AuthViewModel.kt][kotlin]{
class AuthViewModel : ViewModel(), FirebaseAuth.AuthStateListener {
    private lateinit var auth: FirebaseAuth
    private lateinit var db: FirebaseFirestore

    override fun onCleared() {
        // あえてActivityが閉じられるまで監視を続ける
        auth.removeAuthStateListener(this)
        super.onCleared()
    }

    override fun onAuthStateChanged(firebaseAuth: FirebaseAuth) {
        putAuthUserData()
    }

    // Activity.onCreate()で呼び出す
    fun onCreated() {
        auth = FirebaseAuth.getInstance()
        auth.addAuthStateListener(this)
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

各認証部分の内容に関しては、以降の章でそれぞれ解説を行います。
全体として言えることですが、今回はViewModelで認証部分の処理をほとんど行うため、サンプルコードもViewModelのコードが大半になります。
