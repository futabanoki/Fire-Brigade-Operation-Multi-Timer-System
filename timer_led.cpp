#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdio>

#include "led-matrix.h"
#include "graphics.h"

using namespace std;
using namespace rgb_matrix;


// =====================================================
// 現在時刻 milliseconds
// =====================================================

long long nowMillis() {

    return chrono::duration_cast<
        chrono::milliseconds
    >(
        chrono::system_clock::
            now().
            time_since_epoch()
    ).count();
}


// =====================================================
// /tmp/timer.json 読み込み
// =====================================================

string getApiData() {

    ifstream ifs("/tmp/timer.json");

    if (!ifs) {
        return "";
    }

    stringstream buffer;
    buffer << ifs.rdbuf();

    return buffer.str();
}


// =====================================================
// JSONから数値取得
// =====================================================

long long getNumber(
    const string &json,
    const string &key
) {

    string search =
        "\"" + key + "\":";

    size_t pos =
        json.find(search);

    if (pos == string::npos) {
        return 0;
    }

    pos += search.length();

    return atoll(
        json.c_str() + pos
    );
}


// =====================================================
// 状態取得
// =====================================================

string getState(
    const string &json
) {

    if (
        json.find(
            "\"state\":\"RUN\""
        ) != string::npos
    ) {
        return "RUN";
    }

    if (
        json.find(
            "\"state\":\"STOP\""
        ) != string::npos
    ) {
        return "STOP";
    }

    if (
        json.find(
            "\"state\":\"WAIT\""
        ) != string::npos
    ) {
        return "WAIT";
    }

    return "UNKNOWN";
}


// =====================================================
// 文字列を中央表示するX座標計算
// =====================================================

int getCenteredX(
    Font &font,
    const string &text,
    int canvasWidth
) {

    int width = 0;

    for(char c : text) {
        width += font.CharacterWidth(c);
    }

    int x =
        (canvasWidth - width) / 2;

    if (x < 0) {
        x = 0;
    }

    return x;
}


// =====================================================
// main
// =====================================================

int main(
    int argc,
    char *argv[]
) {

    // =================================================
    // 1. フォントを最初にロード
    // =================================================

    const char *font_path =
        "/home/mu2ura-1/rpi-rgb-led-matrix/fonts/ipa36.bdf";

    cout
        << "Font path = "
        << font_path
        << endl;

    Font font;

    if (!font.LoadFont(font_path)) {

        cerr
            << "Font load failed : "
            << font_path
            << endl;

        return 1;
    }

    cout
        << "Font load OK"
        << endl;


    // =================================================
    // 2. LEDマトリックス初期値
    //
    // コマンドライン指定があれば上書きされる
    // =================================================

    RGBMatrix::Options defaults;

    defaults.rows = 32;
    defaults.cols = 64;
    defaults.chain_length = 2;

    defaults.hardware_mapping =
        "regular";

    // 時計用途なので色深度を少し下げる
    defaults.pwm_bits = 7;


    RuntimeOptions runtime_defaults;

    // rootでGPIO初期化後、
    // 権限を落とす標準動作
    runtime_defaults.drop_privileges = 1;


    // =================================================
    // 3. コマンドラインオプション読み込み
    // =================================================

    RGBMatrix *matrix =
        RGBMatrix::CreateFromFlags(
            &argc,
            &argv,
            &defaults,
            &runtime_defaults
        );


    if (matrix == nullptr) {

        cerr
            << "Matrix initialization failed"
            << endl;

        PrintMatrixFlags(
            stderr,
            defaults,
            runtime_defaults
        );

        return 1;
    }


    // =================================================
    // 4. ダブルバッファ
    // =================================================

    FrameCanvas *canvas =
        matrix->CreateFrameCanvas();


    // =================================================
    // 色
    // =================================================

    Color waitColor(
        255,
        255,
        255
    );

    Color runColor(
        255,
        255,
        0
    );

    Color stopColor(
        255,
        0,
        0
    );


    // =================================================
    // 状態管理
    // =================================================

    string state =
        "WAIT";

    string previousState =
        "";

    long long startTime =
        0;

    // 最後に取得したJSON確認時刻
    long long lastJsonCheck =
        0;

    // STOP後に表示する最終タイム
    double finalTime =
        0.0;

    // 現在RUNか
    bool running =
        false;

    // 前回描画文字列
    // 同じ表示なら描画しない
    string lastDisplayedText =
        "";

    string lastDisplayedState =
        "";


    // =================================================
    // canvasサイズ
    // =================================================

    const int canvasWidth =
        matrix->width();

    const int baselineY = 30;


    cout
        << "Matrix size = "
        << matrix->width()
        << " x "
        << matrix->height()
        << endl;


    // =================================================
    // メインループ
    // =================================================

    while (true) {

        long long now =
            nowMillis();


        // =================================================
        // timer.json確認
        //
        // ローカルファイルなので200ms間隔
        // =================================================

       if (
            now - lastJsonCheck
            >= 200
        ) {

            string json =
                getApiData();


            if (!json.empty()) {

                string newState =
                    getState(json);

                long long apiStart =
                    getNumber(
                        json,
                        "startTime"
                    );

                long long apiStop =
                    getNumber(
                        json,
                        "stopTime"
                   );


                // -----------------------------------------
                // RUN
                // -----------------------------------------

                if (
                    newState == "RUN"
                    &&
                    apiStart > 0
                ) {

                    // 新しいSTARTを検知
                    if (
                        state != "RUN"
                        ||
                        startTime != apiStart
                    ) {

                        startTime =
                            apiStart;

                        finalTime =
                            0.0;

                        running =
                            true;

                        cout
                            << "START detected : "
                            << startTime
                            << endl;
                    }

                    state =
                        "RUN";
                }


                // -----------------------------------------
                // STOP
                // -----------------------------------------

                else if (
                    newState == "STOP"
                    &&
                    apiStart > 0
                    &&
                    apiStop > 0
                ) {

                    finalTime =
                        (apiStop - apiStart)
                        / 1000.0;

                    if(finalTime < 0){
                        finalTime = 0;
                    }

                    cout
                        << "STOP detected : "
                        << finalTime
                        << endl;

                    running = false;
                    state = "STOP";
                }


                // -----------------------------------------
                // WAIT
                // -----------------------------------------

                else if (
                    newState == "WAIT"
                ) {

                    // STOP後の最終値は
                    // WAITに変わっても保持しておく
                    running =
                        false;

                    state =
                        "WAIT";
                }
            }

            lastJsonCheck =
                now;
        }if (now - lastJsonCheck >= 200) {

    string json =
        getApiData();

    cout << "JSON = "
         << json
         << endl;

    if (!json.empty()) {

        string newState =
            getState(json);

        long long apiStart =
            getNumber(
                json,
                "startTime"
            );

        long long apiStop =
            getNumber(
                json,
                "stopTime"
            );

        cout << "newState = "
             << newState
             << endl;

        cout << "apiStart = "
             << apiStart
             << endl;


        // =========================
        // RUN
        // =========================

        if (
            newState == "RUN"
            &&
            apiStart > 0
        ) {

            if (
                state != "RUN"
                ||
                startTime != apiStart
            ) {

                startTime =
                    apiStart;

                finalTime = 0.0;
                running = true;

                cout
                    << "START detected : "
                    << startTime
                    << endl;
            }

            state = "RUN";
        }


        // =========================
        // STOP
        // =========================

        else if (
            newState == "STOP"
        ) {

            if (
                state == "RUN"
                &&
                startTime > 0
            ) {

                finalTime =
                    (now - startTime)
                    / 1000.0;

                if (finalTime < 0) {
                    finalTime = 0;
                }

                cout
                    << "STOP detected : "
                    << finalTime
                    << endl;
            }

            running = false;
            state = "STOP";
        }


        // =========================
        // WAIT
        // =========================

        else if (
            newState == "WAIT"
        ) {

            running = false;
            state = "WAIT";
        }
    }

    lastJsonCheck = now;
}


// =====================================================
// 表示内容を決定
// =====================================================

string displayText;
Color *displayColor = &waitColor;


// -----------------------------------------
// RUN
// -----------------------------------------

if (state == "RUN") {

    displayText = "計測中";
    displayColor = &runColor;

}


// -----------------------------------------
// STOP
// -----------------------------------------

else if (state == "STOP") {

    char buf[32];

    snprintf(
        buf,
        sizeof(buf),
        "%.1f秒",
        finalTime
    );

    displayText = buf;
    displayColor = &stopColor;

}


// -----------------------------------------
// WAIT
// -----------------------------------------

else {

    displayText = "WAIT";
    displayColor = &waitColor;

}


// =====================================================
// 表示が変わったときだけ描画
// =====================================================

if (
    displayText != lastDisplayedText
    ||
    state != lastDisplayedState
) {

    canvas->Clear();

    // まず中央揃えを使わず固定位置でテスト
    int x = 5;

    DrawText(
        canvas,
        font,
        x,
        30,
        *displayColor,
        nullptr,
        displayText.c_str()
    );

    canvas =
        matrix->SwapOnVSync(
            canvas
        );

    cout
        << "DISPLAY = "
        << displayText
        << endl;

    lastDisplayedText =
        displayText;

    lastDisplayedState =
        state;
}

        // =================================================
        // LED表示更新
        //
        // 50ms = 20fps
        // =================================================

        this_thread::sleep_for(
            chrono::milliseconds(200)
        );
    }


    delete matrix;

    return 0;
}
