function getTimerApi() {

  const sheet =
    SpreadsheetApp
      .getActiveSpreadsheet()
      .getSheetByName("Control");

  const control =
    Number(sheet.getRange("B1").getValue()) || 0;

  const stopTime =
    Number(sheet.getRange("C1").getValue()) || 0;

  const savedStartTime =
    Number(sheet.getRange("D1").getValue()) || 0;

  let state = "WAIT";

  if(control > 0){
    state = "RUN";
  }

  if(control === -1){
    state = "STOP";
  }

  const result = {

    state: state,

    // STOP後も本来のSTART時刻を返す
    startTime:
      state === "RUN"
        ? control
        : savedStartTime,

    stopTime: stopTime,

    serverTime: Date.now()

  };

  return ContentService
    .createTextOutput(
      JSON.stringify(result)
    )
    .setMimeType(
      ContentService.MimeType.JSON
    );
}

function doGet(e) {

    // ===== LEDマトリックス用API =====
  if (
    e &&
    e.parameter &&
    e.parameter.api === "timer"
  ) {
    return getTimerApi();
  }

  // ===== 既存のスマホ画面 =====
  const role =
    e?.parameter?.role || "commander";

  const template =
    HtmlService.createTemplateFromFile("index");

  template.role = role;

  return template.evaluate()
    .setTitle("消防操法計測システム");
}

function getCompetitors() {

  const sheet =
    SpreadsheetApp.getActiveSpreadsheet()
      .getSheetByName("Competitors");

  const values =
    sheet.getDataRange().getValues();

  values.shift();

  return values;
}

function saveResult(data) {

  const sheet =
    SpreadsheetApp.getActiveSpreadsheet()
      .getSheetByName("Results");

  sheet.appendRow([
    new Date(),
    data.team,
    data.role,
    data.total,
    data.lap1 || "",
    data.lap2 || "",
    data.lap3 || "",
    data.lap4 || "",
    data.lap5 || "",
    data.lap6 || ""
  ]);

  return true;
}

function getSummary() {

  const sheet =
    SpreadsheetApp
      .getActiveSpreadsheet()
      .getSheetByName("Results");

  const values =
    sheet.getDataRange().getValues();

  values.shift();   // 見出し削除

  const result = {};

  values.forEach(r => {

    const team = r[1];
    const role = r[2];

    if(!result[team]){
      result[team] = {};
    }

    // 後に読んだものが最新になる
    result[team][role] = {

      lap1:r[4],
      lap2:r[5],
      lap3:r[6],
      lap4:r[7],
      lap5:r[8],
      lap6:r[9]

    };

  });

  return result;

}

function reserveStart(exactStartTime) {

  const sheet =
    SpreadsheetApp
      .getActiveSpreadsheet()
      .getSheetByName("Control");

  let start =
    Number(exactStartTime);

  if(!start || start <= 0){
    start = Date.now();
  }

  sheet.getRange("B1").setValue(start);
  sheet.getRange("C1").setValue(0);
  sheet.getRange("D1").setValue(start);

  SpreadsheetApp.flush();

  return start;
}

function getStartTime() {

  const sheet =
    SpreadsheetApp.getActiveSpreadsheet()
      .getSheetByName("Control");

  return Number(
    sheet.getRange("B1").getValue()
  );
}

function resetStart() {

  const sheet =
    SpreadsheetApp
      .getActiveSpreadsheet()
      .getSheetByName("Control");

  sheet.getRange("B1")
       .setValue(0);

  //return true;
}

function stopAll(exactStopTime) {

  const sheet =
    SpreadsheetApp
      .getActiveSpreadsheet()
      .getSheetByName("Control");

  let stopTime =
    Number(exactStopTime);

  // 万一スマホから正常な値が来なかった場合
  if(
    !stopTime ||
    stopTime <= 0
  ){
    stopTime = Date.now();
  }

  sheet.getRange("B1")
       .setValue(-1);

  sheet.getRange("C1")
       .setValue(stopTime);

  SpreadsheetApp.flush();

  return stopTime;
}
