/*
 * ESP32 camera web UI - client-side logic (de-minified from the original).
 *
 * How it talks to the ESP32 (all plain HTTP GET requests):
 *   GET  <origin>/status              -> JSON with the camera's current settings
 *   GET  <origin>/control?var=X&val=Y -> change setting X to value Y
 *   GET  <origin>/capture?_cb=<time>  -> a single JPEG still
 *   GET  <origin>:81/stream           -> MJPEG video stream (separate server on port 81)
 *
 * The big idea: every settings control in the HTML has class "default-action" and an
 * id that is the name of the camera setting (e.g. id="brightness"). When the user
 * changes a control, we send  /control?var=<id>&val=<value>.
 */
document.addEventListener('DOMContentLoaded', function () {

  // Base URL of the ESP32, e.g. "http://192.168.1.50"
  var baseHost = document.location.origin;

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  // Send one setting change to the ESP32.
  // The value depends on the kind of control that triggered it.
  function sendControl(el) {
    let value;

    switch (el.type) {
      case 'checkbox':
        value = el.checked ? 1 : 0;        // switches become 1 / 0
        break;
      case 'range':                        // sliders
      case 'select-one':                   // dropdowns
        value = el.value;
        break;
      case 'button':
      case 'submit':
        value = '1';                       // buttons just send "1" (e.g. Enroll Face)
        break;
      default:
        return;                            // anything else: do nothing
    }

    const url = `${baseHost}/control?var=${el.id}&val=${value}`;
    fetch(url).then(response => {
      console.log(`request to ${url} finished, status: ${response.status}`);
    });
  }

  // Show / hide an element via the .hidden CSS class
  const hide = el => { el.classList.add('hidden'); };
  const show = el => { el.classList.remove('hidden'); };

  // Grey-out / re-enable a button
  const disable = el => { el.classList.add('disabled'); el.disabled = true; };
  const enable  = el => { el.classList.remove('disabled'); el.disabled = false; };

  // Set a control to `value` from code.
  //   updateRemote = true  (default): also send the change to the ESP32 if the value changed
  //   updateRemote = false          : only update the UI (used when loading /status), and
  //                                   show/hide the dependent controls accordingly
  const setControl = (el, value, updateRemote) => {
    updateRemote = (updateRemote === undefined || updateRemote === null) ? true : updateRemote;

    let oldValue;
    if (el.type === 'checkbox') {
      oldValue = el.checked;
      value = !!value;                     // force to true/false
      el.checked = value;
    } else {
      oldValue = el.value;
      el.value = value;
    }

    if (updateRemote && oldValue !== value) {
      sendControl(el);
    } else if (!updateRemote) {
      // Keep dependent controls in sync with the loaded value:
      if (el.id === 'aec') {
        // Auto-exposure on  -> hide manual exposure slider
        value ? hide(aecValueGroup) : show(aecValueGroup);
      } else if (el.id === 'agc') {
        // Auto-gain on -> hide manual gain slider, show gain-ceiling slider
        if (value) { show(gainCeilingGroup); hide(agcGainGroup); }
        else       { hide(gainCeilingGroup); show(agcGainGroup); }
      } else if (el.id === 'awb_gain') {
        // AWB gain on -> show the white-balance mode dropdown
        value ? show(wbModeGroup) : hide(wbModeGroup);
      } else if (el.id === 'face_recognize') {
        // Face recognition on -> allow "Enroll Face"
        value ? enable(faceEnrollBtn) : disable(faceEnrollBtn);
      }
    }
  };

  // ---------------------------------------------------------------------------
  // Grab the elements we need
  // ---------------------------------------------------------------------------
  const streamImg       = document.getElementById('stream');
  const streamContainer = document.getElementById('stream-container');
  const getStillBtn     = document.getElementById('get-still');
  const toggleStreamBtn = document.getElementById('toggle-stream');
  const faceEnrollBtn   = document.getElementById('face_enroll');
  const closeStreamBtn  = document.getElementById('close-stream');

  const agc             = document.getElementById('agc');
  const agcGainGroup    = document.getElementById('agc_gain-group');
  const gainCeilingGroup = document.getElementById('gainceiling-group');

  const aec             = document.getElementById('aec');
  const aecValueGroup   = document.getElementById('aec_value-group');

  const awbGain         = document.getElementById('awb_gain');
  const wbModeGroup     = document.getElementById('wb_mode-group');

  const faceDetect      = document.getElementById('face_detect');
  const faceRecognize   = document.getElementById('face_recognize');
  const framesize       = document.getElementById('framesize');

  // ---------------------------------------------------------------------------
  // Initial load: ask the ESP32 for its current settings and apply them to the UI
  // (without sending anything back)
  // ---------------------------------------------------------------------------
  fetch(`${baseHost}/status`)
    .then(response => response.json())
    .then(status => {
      document.querySelectorAll('.default-action').forEach(el => {
        setControl(el, status[el.id], false);   // status keys match the element ids
      });
    });

  // Little "x" buttons hide whatever panel they're inside
  document.querySelectorAll('.close').forEach(btn => {
    btn.onclick = () => { hide(btn.parentNode); };
  });

  // ---------------------------------------------------------------------------
  // Still image / video stream
  // ---------------------------------------------------------------------------
  const stopStream = () => {
    window.stop();                              // cancels the in-flight stream request
    toggleStreamBtn.innerHTML = 'Start Stream';
  };

  const startStream = () => {
    streamImg.src = `${baseHost + ':81'}/stream`;   // stream is served on port 81
    show(streamContainer);
    toggleStreamBtn.innerHTML = 'Stop Stream';
  };

  getStillBtn.onclick = () => {
    stopStream();
    streamImg.src = `${baseHost}/capture?_cb=${Date.now()}`;  // _cb = cache-buster
    show(streamContainer);
  };

  closeStreamBtn.onclick = () => {
    stopStream();
    hide(streamContainer);
  };

  toggleStreamBtn.onclick = () => {
    const streaming = toggleStreamBtn.innerHTML === 'Stop Stream';
    if (streaming) stopStream(); else startStream();
  };

  faceEnrollBtn.onclick = () => { sendControl(faceEnrollBtn); };

  // ---------------------------------------------------------------------------
  // Default behaviour: any ".default-action" control sends its value on change.
  // (Some controls below override this with their own handler.)
  // ---------------------------------------------------------------------------
  document.querySelectorAll('.default-action').forEach(el => {
    el.onchange = () => sendControl(el);
  });

  // ---------------------------------------------------------------------------
  // Controls that also change what is visible in the UI
  // ---------------------------------------------------------------------------

  // Auto Gain Control: on -> show gain ceiling, hide manual gain; off -> the reverse
  agc.onchange = () => {
    sendControl(agc);
    if (agc.checked) { show(gainCeilingGroup); hide(agcGainGroup); }
    else             { hide(gainCeilingGroup); show(agcGainGroup); }
  };

  // Auto Exposure (sensor): on -> hide manual exposure slider
  aec.onchange = () => {
    sendControl(aec);
    aec.checked ? hide(aecValueGroup) : show(aecValueGroup);
  };

  // AWB Gain: on -> show the WB Mode dropdown
  awbGain.onchange = () => {
    sendControl(awbGain);
    awbGain.checked ? show(wbModeGroup) : hide(wbModeGroup);
  };

  // ---------------------------------------------------------------------------
  // Face detection / recognition only work at CIF (value 5) or smaller resolution.
  // Framesize values: bigger number = bigger picture (10 = UXGA ... 5 = CIF ... 0 = QQVGA)
  // ---------------------------------------------------------------------------

  // Changing resolution: if it's now bigger than CIF, switch both face features off
  framesize.onchange = () => {
    sendControl(framesize);
    if (framesize.value > 5) {
      setControl(faceDetect, false);
      setControl(faceRecognize, false);
    }
  };

  faceDetect.onchange = () => {
    if (framesize.value > 5) {
      alert('Please select CIF or lower resolution before enabling this feature!');
      setControl(faceDetect, false);           // undo the tick
      return;
    }
    sendControl(faceDetect);
    if (!faceDetect.checked) {
      // Turning detection off also turns recognition off and disables enrolling
      disable(faceEnrollBtn);
      setControl(faceRecognize, false);
    }
  };

  faceRecognize.onchange = () => {
    if (framesize.value > 5) {
      alert('Please select CIF or lower resolution before enabling this feature!');
      setControl(faceRecognize, false);        // undo the tick
      return;
    }
    sendControl(faceRecognize);
    if (faceRecognize.checked) {
      // Recognition needs detection, so switch that on too and allow enrolling
      enable(faceEnrollBtn);
      setControl(faceDetect, true);
    } else {
      disable(faceEnrollBtn);
    }
  };
});
