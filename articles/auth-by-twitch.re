={auth-by-twitch} カスタム認証システムを使ってTwitchアカウントでAuthenticationを利用する

//lead{
カスタム認証システムを利用することで、自由な形で認証処理を実現できます。

今回はOAuthProviderでサポートされていないTwitchというWebサービスのアカウントを、カスタム認証システムを使って対応する方法を解説します。
//}

== カスタム認証方式について

GoogleアカウントやOAuthProviderによる認証処理はとても便利ですが、作るサービスによっては、それ以外のWebサービスアカウントから認証したくなることがあるかと思います。
この場合、そのWebサービスが提供してくれるOAuthとFirebase Admin SDKを利用したカスタム認証システムを使うことで、認証を実現できます。

Firebase公式ブログではLINEを使った場合のカスタム認証システムの例が解説されていますが@<fn>{link-developer-blog-line-auth}@<fn>{note-developer-blog-line-auth}、本書ではTwitchというゲーム配信サービスのアカウントを使った例を解説します。

//footnote[link-developer-blog-line-auth][https://developers-jp.googleblog.com/2016/11/authenticate-your-firebase-users-with-line-login.html]

//footnote[note-developer-blog-line-auth][サンプルコードには、Instagramとカカオトークの例も掲載されています。]

===[column] Twitchとは

Twitchは主にゲームに特化したライブストリーミング配信サービスです。
現在はAmazonに買収されています。
Evoなどのゲーム大会の配信媒体でお馴染みのサイトでもあり、ユーザー数もとても多いサービスです。

===[/column]

=== 諸注意

カスタム認証システムは、クライアント（Androidプロジェクト）だけでは実現ができません。
セキュリティ的な問題という以前に、カスタム認証によるユーザー作成を実現するAPIが提供されていないためです。
そのため、FirebaseUser作成にはサーバサイド用に提供されているFirebase Admin SDKを利用することになり、必然的にサーバを用意する必要があります。

本書ではサーバ部分をCloud Functionsを使うことで対応しています。
なお、無料プランではCloud Functionsから外部のURLでアクセスができません。
今回は検証ということで利用分が無料枠分で収まる範囲なので、従量課金プラン（Blazeプラン）を設定しています。

== Twitchにアプリケーションを登録する

まず準備段階として、Twitterと同様にデベロッパーサイト@<fn>{url-twitch-developer-apps}からアプリケーションの登録が必要になります。
当然ユーザーはデベロッパー登録をする必要がありますが、ユーザーアカウントが二段階認証済みである必要があります。
それ以外は特につまずくことはありません。

//image[twitch_developer_settings][アプリケーション登録画面][scale=0.65]{
//}

カテゴリはなんでもよいと思います。
今回は「Application Integration」としました。

OAuthのリダイレクトURLは今回利用します。
きちんとしたものを入力するようにしましょう。
登録が完了すると、アプリケーションごとにクライアントIDが付与されています。

//image[twitch_developer_done][アプリケーション登録完了後、クライアントIDが付与される][scale=0.65]{
//}

これで準備は完了しました。
今回はカスタム認証システムを使うことになるため、Firebaseのコンソールで何かする必要はありません。

//footnote[url-twitch-developer-apps][https://dev.twitch.tv/console/apps/create]

== 今回のカスタム認証の流れ

コードを紹介する前に、今回の認証の簡単な流れを解説します。

//image[auth-by-twitch-chart][認証の流れ][scale=0.9]{
//}

まずTwitchのOAuth認証画面に飛んで、ユーザーのアクセストークンを取得するまでをクライアント（Androidアプリ）側で行います。
そして取得したアクセストークンをCloud Functions（サーバ）に送り、あらためてアクセストークンの検証をするとともに、Twitchのアカウントユーザー情報の取得を行います。
ここまで問題なく取得ができていれば、Firebase Admin SDKを使ってFirebase Authenticationへの認証を行います。
認証が完了した後は、TwitchではなくFirebaseUserの方のアクセストークンをクライアントへ返却します。
クライアントは受け取ったアクセストークンを使って、Firebase Authenticationにサインインする、というのがカスタム認証の流れになります。

この大まかな流れは、ほかのアカウントのOAuthを使って実装する場合もほぼ同じ流れになるかと思います。

== Twitchからアクセストークンを取得する

まずはクライアントからTwitchのOAuth認証画面を開き、アクセストークンを取得するまでを実装します。
TwitchのサイトはCustomTabsを使うことをお勧めします。
むしろそのためにあるのがCustomTabsです。

//list[open-twitch-customtabs][AuthTwitchActivity.kt][kotlin]{
// implementation "androidx.browser:browser:1.0.0" の追加を忘れないこと

sign_in_button.setOnClickListener {
    CustomTabsIntent.Builder()
        .build()
        .launchUrl(this@AuthTwitchActivity, authorizeUri())
}

companion object {
    private fun authorizeUri() = "https://id.twitch.tv/"
        .plus("oauth2/authorize")
        .plus("?client_id=" + /*クライアントID*/ )
        .plus("&redirect_uri=" + /*リダイレクトURL*/)
        .plus("&response_type=token")
        .plus("&scope=user:edit")
        .toUri()
}
//}

これでサインインボタンをタップするとTwitchのOAuth認証画面に遷移します。

//image[oauth-twitch][TwitchのOAuth認証画面][scale=0.35]{
//}

しかしこれだけではクライアント側でコールバックを受け取れません。
AndroidManifestで、コールバックを受け取れるように設定を追加します。

//list[add-twitch-callback-android-manifest][AndroidManifest.xml][xml]{
<activity android:name=".ui.auth.signin.AuthTwitchActivity"
            android:launchMode="singleTask">
    <intent-filter>
        <action android:name="android.intent.action.VIEW"/>

        <category android:name="android.intent.category.DEFAULT"/>
        <category android:name="android.intent.category.BROWSABLE"/>

        <data
            android:host="oauth"
            android:scheme="io.github.yamacraft.app.sampra"/>
    </intent-filter>
</activity>
//}

今回は認証部分をひとつのActivity内で完結させたかったので、SingleTaskにして設定しました。
コールバックURLを @<code>{io.github.yamacraft.app.sampra://oauth} と設定していたので、hostに @<code>{oauth} 、schemeに @<code>{io.github.yamacraft.app.sampra} と設定しています。
これでOAuth認証が完了すると再度呼び出し元のAuthTwitchActivityが立ち上がり直し、 @<code>{onNewIntent()} にコールバックされたURIが格納された状態で呼ばれます。

//list[getting-over-onNewIntent][AuthTwitchActivity.kt][kotlin]{
override fun onNewIntent(intent: Intent) {
    super.onNewIntent(intent)

    intent.data?.let {
        viewModel.onCallbackUri(it)
    }
}
//}

URI取得からどうやってFirebaseユーザーにサインインするかは、Activity側で知る必要はありません。
以降はViewModel側に処理を任せます。

=== 返却されたURIからアクセストークンを抜き出す

返却されたURIからアクセストークンを取り出したいのですが、ここでTwitch特有の問題があります。
それはURIの形式が @<code>{xxx://yyy#access_token=xxxxxxxxxx&scope=user:edit&token_type=bearer} となっていて、そのままでは @<code>{URI.getQueryParameter()} で値が取ることができません。
少々強引な手になりますが、先にこの＃を？に置き換えてから、アクセストークンを取り出すことにします。

//list[replace-twitch-oauth-uri][AuthTwitchViewModel.kt][kotlin]{
fun onCallbackUri(uri: Uri) {
    val accessToken = uri.toString()
        .replace("oauth#", "oauth?")
        .toUri()
        .getQueryParameter("access_token")
        .orEmpty()
}
//}

これでアクセストークンが取得できました。
試しにサーバ側で検証に使うverify用のエンドポイントを、curlから叩いて確認してみます。

//list[curl-twitch-accesstoken-verify][curlでアクセストークン検証][sh]{
curl -H "Authorization: OAuth xxxxxxxx" https://id.twitch.tv/oauth2/validate

{
  "client_id":"xxxxxxxxxxxxxxxxxxxxx",
  "login":"yamacraft",
  "scopes":["user:edit"],
  "user_id":"99999999"
}
//}

問題なくアクセストークンを取得したことを確認できました。

== サーバサイドからFirebase Authenticationの認証処理を実装する

次はサーバサイドの処理を実装します。
サーバでは主に以下の作業を行います。

 * 受け取ったトークンを @<code>{https://id.twitch.tv/oauth2/validate} で検証し、合わせてユーザー情報を取得する
 * TwitchのUIDをもとに、すでに該当のFirebaseUserを取得
 ** もし登録がなければ新規作成する
 * 該当のFirebaseUserにサインインするためのアクセストークンを返却する

=== Cloud Functions 全体のコード

申し訳ありませんが、筆者はTypeScriptもnodejsもド素人ですので、Cloud Functions内のコードは公式が提供しているサンプル@<fn>{link-sample-custom-auth-server}をTwitch用に変更し、tsでビルドできるようにした程度のものになります。

全体の詳しい解説は行いません。
代わりにコードをそのまま掲載するので、フィーリングで何かを感じとってもらえれば幸いです。

//list[custom-auth-functions-code][index.ts][ts]{
'use strict';

import * as functions from 'firebase-functions';
import { FirebaseError } from 'firebase-admin';

// Modules imports
const rp = require('request-promise');
const express = require('express');
const bodyParser = require('body-parser');

// Firebase Setup
// service-account.jsonはlib配下に置く
const admin = require('firebase-admin');
const serviceAccount = require('./service-account.json');
admin.initializeApp({
  credential: admin.credential.cert(serviceAccount)
});

function generateTwitchApiRequest(apiEndpoint: string, accessToken: string) {
    return {
        url: apiEndpoint,
        headers: {
            'Authorization': `OAuth ${accessToken}`
        },
        json: true
    };
}

// Firebase登録情報の取得
function getFirebaseUser(uid: string, accessToken: string) {
    return admin.auth().getUser(uid).catch((error: FirebaseError) => {
        // まだ作成されていない場合は、FirebaseUserを作成
        if (error.code === 'auth/user-not-found') {
            const getProfileOptions 
                = generateTwitchApiRequest(
                    'https://id.twitch.tv/oauth2/validate', accessToken);
            return rp(getProfileOptions)
                .then((response: {login:string, user_id:string}) => {
                    const twitchUid = response.user_id;
                    const displayName = response.login;            

                    // 作成したUser情報を登録して返却
                    return admin.auth().createUser({
                        uid: twitchUid,
                        displayName: displayName
                    });
            });
        }
        throw error;
    });
}

// accessToken検証
function verifyTwitchToken(accessToken: string) {

    // 外部アクセスするので有料プラン（Flame or Blaze）必須
    const verifyTokenOptions 
        = generateTwitchApiRequest(
            'https://id.twitch.tv/oauth2/validate', accessToken);
    return rp(verifyTokenOptions)
        .then((response: { user_id: string; }) => {
            // 本来ならClientIdのチェックを行う
            const uid = response.user_id;
            return getFirebaseUser(uid, accessToken);
        })
        .then((userRecord: { uid: string; }) => {
            // Firebase AuthenticationのaccessTokenを取得して返す
            const tokenPromise = 
                admin.auth().createCustomToken(userRecord.uid);
            tokenPromise.then( (token: string) => {
            });
        return tokenPromise;
    });
}

// ExpressJS setup
const app = express();
app.use(bodyParser.json());

// POST /verifyTwitch
export const verifyTwitch = functions.https.onRequest((request, response) => {
    if(request.body.token === undefined) {
        const ret = {
            error_message: 'Access Token not found'
        };
        return response.status(400).send(ret);
    }

    const reqToken = request.body.token;

    verifyTwitchToken(reqToken)
        .then((customAuthToken: string) => {
            const ret = {
                firebase_token: customAuthToken
            };
            return response.status(200).send(ret);   
        })
        .catch((err: FirebaseError)=> {
            const ret = {
                error_message: 'Cannot verify access token.'
            };
            return response.status(403).send(ret);
        });

    return response.status(400);
});
//}

こちらもクライアントから取得したTwitchのアクセストークンを使ってcurlから検証します。
Cloud FunctionsのURLは、Firebaseへのデプロイ時に返却値として確認できます。

//list[curl-check-functions][curlでFunctionsの動作検証][sh]{
curl -X POST -H "Content-Type: application/json" \
    -d '{"token":"xxxxxxxxxxxxxxxxxxx"}' \
    https://xxxxxxxx.cloudfunctions.net/verifyTwitch

{
    "firebase_token":"abCDe...."
}
//}

無事に動作することが確認できました。
実際のfirebase_tokenは果てしない長さの文字列です。
もしまだ認証したことのないTwitchアカウントだった場合、コンソール画面にFirebaseUserが追加されていることを確認できます。

//image[created_firebase_auth_twitch][追加されたFirebaseUser][scale=0.65]{
//}

//footnote[link-sample-custom-auth-server][https://github.com/FirebaseExtended/custom-auth-samples/blob/master/Line/server/app.js]

=== Cloud Functions側で気を付けること

Cloud FunctionsのHTTPトリガは、それ自体にアクセス制限をかけることができません。
curlで確認できたように、URLさえ分かっていればどこからでもアクセスできてしまいます。
そのため今回の実装では、Twitchのアクセストークンとして問題がなければ、どのアプリケーション経由から取得したアクセストークンでも認証が通ってしまいます。
ですのでコード内のコメントでも書きましたが、verifyで返すclient_idの中身を検証する必要があります。
実運用の際には、このあたりも含めサーバ部分の実装は十分留意するようにしてください。

=== Firebase Admin SDKによるFirebaseUser作成の懸念点

サーバで実際にFirebse Userを生成している部分は @<code>{getFirebaseUser()} 内の次の部分です。

//list[create-custom-auth-user][index.ts][ts]{
return admin.auth().createUser({
    uid: twitchUid,
    displayName: displayName
});
//}

通常、クライアント側だけで解決する登録処理ではUIDを設定することはできませんが、Firebase Admin SDKならUIDを指定してユーザーを作成できます。
今回の処理では、TwitchのUIDをそのままFirebse UserのUIDとしても使う形をとっています。

本当にセキュアな実装をするのであれば、TwitchのUIDとFirebaseUserのUIDは別とすべきです。
たとえばFireStoreでTwitchUIDとFirebaseUserUIDの紐付け情報のみを保存するコレクションを作り、アクセス権限を認証関係なくすべてfalseにしてしまいます。
こうすることで、少なくともクライアント側からは完全にアクセス不可能で、Firebase Admin SDKを使ったサーバからしかアクセスできないようになります。

この上で、

 * 紐付けコレクションから、取得したTwitchユーザーがすでに登録済みであるか確認
 ** あるのなら、そのFirebaseUserのアクセストークンを返却
 * uidを指定せずに、 @<code>{createUser()} を実施（UIDは自動で割り振られる）
 * 返却されたFirebaseUserのUIDとTwitchのUIDの紐付け情報をコレクションに登録
 * クライアントにFirebaseUserのアクセストークンを返却

という実装が、本来あるべき姿ではないかと考えます。
ただ今回はあくまでも動作検証までとしているので、TwitchのUIDをそのままFirebaseUserのUIDとして利用しています。

== クライアントからサーバにRetfofit＋Coroutineでアクセスする

とりあえずサーバ側の実装は完了しました。
クライアントの実装に戻って、アクセストークン取得後にサーバへ送る処理を実装していきます。
今回はRetrofit2+Coroutineで実装しています。
特にこれらの解説はありません。

//list[create-api-retrofit][AuthFunctionApi.kt][kotlin]{
interface AuthFunctionApi {
    @FormUrlEncoded
    @POST("verifyTwitch")
    fun verify(@Field("token") token: String):
        Call<FunctionVerifyTwitchResponse>
}
//}

//list[create-apiclient-retrofit][AuthFunctionApiClient.kt][kotlin]{
object AuthFunctionApiClient {
    private const val BASE_URL = BuildConfig.FUNCTIONS_BASE_URL
    private const val TIMEOUT_MINUTES = 1L

    fun create(): AuthFunctionApi = Retrofit.Builder()
        .baseUrl(BASE_URL)
        .client(createOkHttpClient())
        .addConverterFactory(createGsonConverterFactory())
        .build()
        .create(AuthFunctionApi::class.java)

    private fun createOkHttpClient() = OkHttpClient().newBuilder()
        .connectTimeout(TIMEOUT_MINUTES, TimeUnit.MINUTES)
        .readTimeout(TIMEOUT_MINUTES, TimeUnit.MINUTES)
        .addInterceptor(createHttpLoggingInterceptor())
        .build()

    private fun createHttpLoggingInterceptor() = HttpLoggingInterceptor()
        .setLevel(HttpLoggingInterceptor.Level.HEADERS)

    private fun createGsonConverterFactory() = GsonConverterFactory.create(
        GsonBuilder()
            .setFieldNamingPolicy(
                FieldNamingPolicy.LOWER_CASE_WITH_UNDERSCORES)
            .create()
    )
}
//}

//list[send-verify-accesstoken][AuthTwitchViewModel.kt][kotlin]{
class AuthTwitchViewModel : ViewModel(), CoroutineScope {

    private lateinit var auth: FirebaseAuth
    private var validateJob: Job? = null

    override fun onCleared() {
        validateJob?.cancel()
        super.onCleared()
    }

    fun onCallbackUri(uri: Uri) {
        val accessToken = uri.toString()
            .replace("oauth#", "oauth?")
            .toUri()
            .getQueryParameter("access_token")
            .orEmpty()

        validateJob = launch {
            val response = AuthFunctionApiClient
                .create()
                .verify(accessToken)
                .execute()
            withContext(Dispatchers.Main) {
                if (response.isSuccessful) {
                    response.body()?.let {
                        firebaseSignIn(it.firebaseToken)
                    }
                }
            }
        }
    }

    private fun firebaseSignIn(token: String) {
        auth.signInWithCustomToken(token)
            .addOnCompleteListener {
                if (it.isSuccessful) {
                    // 認証完了
                } else {
                    // 認証失敗
                }
            }
    }
}
//}

最終的に完成した、ViewModelとApiClient関連の処理です。
最後に受け取ったFirebaseUserのアクセストークンを使って @<code>{signInWithCustomToken()} することで、クライアント側での認証が完了します。

主にサーバも含み、これまでの認証方式に比べるとコード量は一気に増えるものの、内容自体は実にシンプルです。
ただし間に独自サーバを介すようになったことで、サインインの待ち時間は体感でも明らかに伸びています。

前述のUIDに関する懸念点もありますし、カスタム認証を採用する際はこれらを十分考慮するようにしてください。

== カスタム認証システムの強さと怖さ

今回はOAuthを経由しての認証となりましたが、実際の動作を見るとわかるように、Firebase Admin SDK自身はOAuth経由だろうがなんだろうが、その点を気にせずユーザーを作成できてしまいます。

たとえば、いきなりFirebaseUserを作成してUIDを直接返し、ユーザーはUIDを直接入力して送信することで認証するというシステムも、Firebase Admin SDKなら作れてしまいます。@<fn>{example-custom-tenho}

自由に作成できる反面、セキュリテイ的な穴も作れてしまいがちです。
繰り返しになってしまいますが、サーバ部分の実装は十分気をつけましょう。

//footnote[example-custom-tenho][たとえばオンライン麻雀ゲームの天鳳では、ほぼほぼこの形式でユーザー認証を行っています。]