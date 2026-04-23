/* === Settings Page Logic === */
var settingsLoaded = false;
var PWD_SENTINEL = '\u25CF\u25CF\u25CF\u25CF\u25CF\u25CF'; // 6 ● shown as dots in password field

function onPwdInput(el) {
  var btn = document.getElementById('pwdEyeBtn');
  if (!btn) return;
  var modified = el.value.length > 0 && el.value !== PWD_SENTINEL;
  btn.style.display = modified ? 'inline-block' : 'none';
  if (!modified) el.type = 'password'; // reset to hidden if cleared
}

function togglePwdVisibility() {
  var inp = document.getElementById('s_WifiPSWD');
  var btn = document.getElementById('pwdEyeBtn');
  if (!inp || !btn) return;
  inp.type = (inp.type === 'password') ? 'text' : 'password';
  btn.innerHTML = inp.type === 'password' ? '&#x1F441;' : '&#x1F648;';
}

// Field map: card name → list of {id, key, password?, int?}
var CARD_FIELDS = {
  wifi: [
    { id: 's_WifiSSID', key: 'WifiSSID' },
    { id: 's_WifiPSWD', key: 'WifiPSWD', password: true },
  ],
  location: [
    { id: 's_Lat',      key: 'Lat' },
    { id: 's_Lon',      key: 'Lon' },
    { id: 's_City',     key: 'City' },
    { id: 's_Timezone', key: 'Timezone' },
  ],
  schedule: [
    { id: 's_SleepMin', key: 'SleepMin', int: true },
    { id: 's_BedTime',  key: 'BedTime',  int: true },
    { id: 's_WakeTime', key: 'WakeTime', int: true },
  ],
  units: [
    { id: 's_UnitsTemp',   key: 'UnitsTemp' },
    { id: 's_UnitsSpeed',  key: 'UnitsSpeed' },
    { id: 's_UnitsPres',   key: 'UnitsPres' },
    { id: 's_UnitsDist',   key: 'UnitsDist' },
    { id: 's_UnitsPrecip', key: 'UnitsPrecip' },
  ],
  display: [
    { id: 's_TimeFmt',  key: 'TimeFmt' },
    { id: 's_DateFmt',  key: 'DateFmt' },
    { id: 's_Language', key: 'Language' },
  ],
};

function _getVal(id) {
  var el = document.getElementById(id);
  return el ? el.value : '';
}

function _setVal(id, val) {
  var el = document.getElementById(id);
  if (!el) return;
  if (el.tagName === 'SELECT') {
    // Try to select matching option
    var opts = el.options;
    for (var i = 0; i < opts.length; i++) {
      if (opts[i].value === String(val)) { el.selectedIndex = i; return; }
    }
    // No match — leave as-is
  } else {
    el.value = (val !== null && val !== undefined) ? String(val) : '';
  }
}

function loadSettingsConfig() {
  fetch('/api/config')
    .then(function(r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.json();
    })
    .then(function(cfg) {
      Object.keys(CARD_FIELDS).forEach(function(card) {
        CARD_FIELDS[card].forEach(function(f) {
          if (!f.password) _setVal(f.id, cfg[f.key]);
        });
      });
      // Show 6 masked dots to indicate a password is stored
      var pwdEl = document.getElementById('s_WifiPSWD');
      if (pwdEl) { pwdEl.value = PWD_SENTINEL; pwdEl.type = 'password'; }
      var eyeBtn = document.getElementById('pwdEyeBtn');
      if (eyeBtn) eyeBtn.style.display = 'none';
      settingsLoaded = true;
    })
    .catch(function(e) {
      showToast('Failed to load config: ' + e, true);
    });
}

function saveCardConfig(card) {
  var fields = CARD_FIELDS[card];
  if (!fields) return;

  var data = {};
  fields.forEach(function(f) {
    var val = _getVal(f.id);
    if (f.password && (!val || val === PWD_SENTINEL)) return; // skip empty / sentinel
    data[f.key] = f.int ? (parseInt(val, 10) || 0) : val;
  });

  // Disable save button during request
  var btn = document.querySelector('#page-settings .card-footer [onclick*="' + card + '"]');
  if (btn) { btn.disabled = true; btn.textContent = 'Saving...'; }

  fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data),
  })
    .then(function(r) { return r.json(); })
    .then(function(res) {
      if (btn) { btn.disabled = false; btn.textContent = 'Save'; }
      if (res.ok) showToast('Saved successfully');
      else showToast('Save failed: ' + (res.error || 'unknown'), true);
    })
    .catch(function(e) {
      if (btn) { btn.disabled = false; btn.textContent = 'Save'; }
      showToast('Request failed: ' + e, true);
    });
}

/* --- City search via Open-Meteo Geocoding API --- */
var citySearchTimer = null;
function searchCity(query) {
  clearTimeout(citySearchTimer);
  var dd = document.getElementById('cityDropdown');
  if (!query || query.length < 2) { dd.style.display = 'none'; return; }
  citySearchTimer = setTimeout(function() {
    fetch('https://geocoding-api.open-meteo.com/v1/search?name=' + encodeURIComponent(query) + '&count=8&language=en&format=json')
    .then(function(r) { return r.json(); })
    .then(function(data) {
      if (!data.results || data.results.length === 0) {
        dd.innerHTML = '<div class="city-item" style="color:#888;">No results</div>';
        dd.style.display = 'block'; return;
      }
      dd.innerHTML = '';
      data.results.forEach(function(c) {
        var item = document.createElement('div');
        item.className = 'city-item';
        item.textContent = c.name + (c.admin1 ? ', ' + c.admin1 : '') + (c.country ? ', ' + c.country : '') + ' (' + c.latitude.toFixed(2) + ', ' + c.longitude.toFixed(2) + ')';
        item.onclick = function() {
          document.getElementById('s_City').value = c.name + (c.admin1 ? ', ' + c.admin1 : '') + (c.country ? ', ' + c.country : '');
          document.getElementById('s_Lat').value = c.latitude.toFixed(4);
          document.getElementById('s_Lon').value = c.longitude.toFixed(4);
          dd.style.display = 'none';
        };
        dd.appendChild(item);
      });
      dd.style.display = 'block';
    }).catch(function() { dd.style.display = 'none'; });
  }, 350);
}

document.addEventListener('click', function(e) {
  var dd = document.getElementById('cityDropdown');
  if (dd && !dd.contains(e.target) && e.target.id !== 's_City') dd.style.display = 'none';
});
