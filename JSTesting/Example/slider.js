let connectButton = document.getElementById("connect");
let statusDisplay = document.getElementById("status");
let slider = document.getElementById("slider");
decoder = new TextDecoder();
let device;

async function loopRead() {
  if (!device) {
    console.log('no device');
    return;
  }

  try { //Simplify if possible
    const result = await device.transferIn(5, 64);
    const command = decoder.decode(result.data);
    const val = parseInt(command.trim());
    slider.value = val;

    loopRead()
  } catch (e) {
    console.log('Error reading data', e);
  }
}

async function disconnect() {
  if (!device) {
    return;
  }
  console.log("Closing")
  await device.controlTransferOut({
    requestType: "class",
    recipient: "interface",
    request: 0x22,
    value: 0x00,
    index: 0x02,
  });
  await device.close();
  console.log("Device Closed!")
  device = null;
  connectButton.innerHTML = "Connect";
}

async function connect() {
  try {
    const newDevice = await navigator.usb.requestDevice({
      filters: [{ 'vendorId': 0x2341, 'productId': 0x8036 }],
    });
    device = newDevice;
    await device.open();
    await device.selectConfiguration(1);
    await device.claimInterface(2);
    await device.selectAlternateInterface(2, 0);
    await device.controlTransferOut({
      requestType: "class",
      recipient: "interface",
      request: 0x22,
      value: 0x01,
      index: 0x02,
    });
    console.log("Successfully connected");
    connectButton.innerHTML = "Disconnect";
    loopRead();
  } catch (e) {
    console.log("Failed to Connect: ", e);
  }
}

connectButton.onclick = () => {
  if (device) {
    disconnect();
  } else {
    connect();
  }
};