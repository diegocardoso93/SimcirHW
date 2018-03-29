
var getBoardsTemplates = function(gpios, option) {
  var HTMLoptions = '<option></option>';
  $.each(gpios, function(k, v) {
    HTMLoptions += '<option>' + k + '</option>';
  });
  var HTMLboard = "";
  switch (option) {
    case 'NodeMCU_ESP32S':
      HTMLboard += `
          <input type="hidden" name="board" value="NodeMCU_ESP32S" />
          <input type="hidden" name="hardware" value="esp32" />
          <div style="float:left;width:80px;text-align:right;margin-right:10px;">
            <select selectType="boardPin" name="VP" style="height:20.5px;margin-top:56px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="VN" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D34" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D35" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D32" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D33" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D25" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D26" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D27" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D14" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D12" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D13" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
          </div>
          `;
      HTMLboard += `<img src="img/NodeMCU_ESP32S.png" style="float:left;height:450px;"/>`;
      HTMLboard += `
          <div style="float:left;width:80px;margin-left:10px;">
            <select selectType="boardPin" name="D23" style="height:20.5px;margin-top:34.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D22" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="TX0" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="RX0" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D21" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D19" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D18" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D5"  style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="TX2" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="RX2" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D4"  style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D2"  style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D15" style="height:20.5px;margin-top:1.5px;width:80px">${HTMLoptions}</select>
          </div>
          `;
      break;
    case 'NodeMCU_V2':
      HTMLboard += `
          <input type="hidden" name="board" value="NodeMCU_V2" />
          <input type="hidden" name="hardware" value="esp8266" />
          <div style="float:left;width:80px;text-align:right;margin-right:10px;">
            <select selectType="boardPin" name="SD3" style="margin-top:138px;width:80px">${HTMLoptions}</select>
          </div>
          `;
      HTMLboard += `<img src="img/NodeMCU_V2.png" style="float:left;height:450px;"/>`;
      HTMLboard += `
          <div style="float:left;width:80px;margin-left:10px;">
            <select selectType="boardPin" name="D0" style="margin-top:47px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D1" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D2" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D3" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D4" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D5" style="margin-top:48px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D6" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D7" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D8" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
          </div>
          `;
      break;
    case 'Wemos_D1':
      HTMLboard += `<img src="img/Wemos_D1.png" style="float:left;height:500px;margin-left:100px"/>`;
      HTMLboard += `
          <input type="hidden" name="board" value="Wemos_D1" />
          <input type="hidden" name="hardware" value="esp8266" />
          <div style="float:left;width:80px;margin-left:10px;">
            <select selectType="boardPin" name="D13" style="margin-top:200px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D12" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D11" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D10" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D9" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D8" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D4" style="margin-top:44px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D3" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
            <select selectType="boardPin" name="D2" style="margin-top:2px;width:80px;height:21px">${HTMLoptions}</select>
          </div>
          `;
      break;
    default:
      HTMLboard = "";
      break;
  }
  return HTMLboard;
}