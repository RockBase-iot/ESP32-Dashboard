#pragma once

// Single-file embedded frontend for the Weather Station web portal.
// Served at GET / — contains all CSS and JS inline to avoid SPIFFS dependency.
// Design references NMMiner's dark-themed sidebar + card layout.

static const char INDEX_HTML[] PROGMEM = R"HTMLEOF(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Weather Station</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;}
html{font-size:16px;height:100%;}
body{font-family:'Segoe UI',Arial,sans-serif;color:#e0e0e0;background:#1a1a2e;min-height:100vh;}
a{text-decoration:none;color:#4CAF50;}

/* --- Toast --- */
.toast-container{position:fixed;bottom:12px;right:12px;z-index:99999;display:flex;flex-direction:column;gap:6px;pointer-events:none;}
.toast{width:300px;border-radius:6px;padding:12px 16px;font-size:0.88rem;pointer-events:auto;opacity:0;transform:translateX(40px);transition:opacity .3s,transform .3s;color:#fff;box-shadow:0 2px 12px rgba(0,0,0,.5);}
.toast.in{opacity:.95;transform:translateX(0);}
.toast.out{opacity:0;transform:translateX(40px);}
.toast.success{background:#388E3C;}
.toast.error{background:#C62828;}
.toast.info{background:#1565C0;}
.toast .toast-title{font-weight:700;margin-bottom:3px;}
.toast .toast-msg{opacity:.88;}

/* --- Layout --- */
.topbar{position:fixed;top:0;left:0;right:0;height:3.5rem;background:linear-gradient(135deg,#1a1a2e,#2a2a40);border-bottom:1px solid #333;display:flex;align-items:center;padding:0 1.5rem;z-index:200;box-shadow:0 2px 8px rgba(0,0,0,.4);}
.logo-text{font-size:1.3rem;font-weight:900;color:#4CAF50;letter-spacing:1px;}
.logo-sub{font-size:.85rem;color:#aaa;margin-left:8px;}
.sidebar{position:fixed;top:3.5rem;left:0;bottom:0;width:220px;background:linear-gradient(180deg,#1e1e32,#16162a);border-right:1px solid #2a2a3e;overflow-y:auto;z-index:100;}
.sidebar::-webkit-scrollbar{width:4px;}
.sidebar::-webkit-scrollbar-thumb{background:#444;border-radius:2px;}
.menu{list-style:none;padding:.5rem 0;}
.menu li{margin:2px 8px;}
.menu li a{display:flex;align-items:center;gap:10px;padding:.7rem 1rem;color:#8e8ea0;border-radius:8px;cursor:pointer;font-size:.92rem;font-weight:500;transition:all .2s;}
.menu li a:hover{background:rgba(76,175,80,.1);color:#fff;}
.menu li a.active{background:rgba(76,175,80,.15);color:#4CAF50;font-weight:700;border-left:3px solid #4CAF50;}
.menu-icon{font-size:1.1rem;width:24px;text-align:center;}
.main{margin-left:220px;margin-top:3.5rem;padding:1.5rem 2rem;min-height:calc(100vh - 3.5rem);}
.page{display:none;}
.page.active{display:block;}

/* --- Cards --- */
.card{background:#1e1e32;border:1px solid #2a2a3e;border-radius:10px;margin-bottom:1.25rem;box-shadow:0 2px 8px rgba(0,0,0,.3);}
.card-title{display:flex;align-items:center;gap:8px;padding:.9rem 1.25rem;font-size:1rem;font-weight:700;color:#e0e0e0;border-bottom:1px solid #2a2a3e;}
.card-icon{font-size:1.15rem;}
.card-body{padding:1rem 1.25rem;}
.card-footer{padding:.75rem 1.25rem;border-top:1px solid #2a2a3e;display:flex;justify-content:flex-end;}
.field-grid{display:flex;align-items:center;padding:.45rem 0;gap:1rem;}
.field-label{flex:0 0 160px;font-size:.88rem;color:#aaa;}
.field-input,.field-select{flex:1;background:#12122a;border:1px solid #3a3a5e;border-radius:6px;color:#e0e0e0;padding:.45rem .75rem;font-size:.88rem;outline:none;transition:border-color .2s;}
.field-input:focus,.field-select:focus{border-color:#4CAF50;}
.field-select option{background:#1e1e32;}
.field-hint{font-size:.76rem;color:#666;margin-top:2px;margin-left:160px;padding-left:1rem;}
.section-label{font-size:.8rem;font-weight:700;color:#4CAF50;text-transform:uppercase;letter-spacing:.5px;padding:.6rem 0 .2rem;}
.btn-save{background:linear-gradient(135deg,#2e7d32,#4CAF50);color:#fff;border:none;padding:.45rem 1.4rem;border-radius:6px;cursor:pointer;font-size:.88rem;font-weight:700;transition:all .2s;}
.btn-save:hover{background:linear-gradient(135deg,#4CAF50,#66BB6A);transform:translateY(-1px);box-shadow:0 3px 10px rgba(76,175,80,.35);}
.btn-save:active{transform:translateY(0);}

/* --- Placeholder --- */
.placeholder{display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:50vh;color:#555;}
.placeholder .ph-icon{font-size:4rem;margin-bottom:1rem;opacity:.35;}
.placeholder .ph-title{font-size:1.4rem;color:#8e8ea0;margin-bottom:.5rem;}
.placeholder .ph-desc{font-size:.88rem;color:#555;}

/* --- Responsive --- */
@media(max-width:640px){
  .sidebar{transform:translateX(-100%);transition:transform .3s;}
  .sidebar.open{transform:translateX(0);}
  .main{margin-left:0;}
  .field-label{flex:0 0 110px;}
  .field-hint{margin-left:110px;}
  .menu-btn{display:flex!important;}
}
.menu-btn{display:none;background:none;border:1px solid #555;border-radius:6px;color:#fff;font-size:1.3rem;padding:5px 9px;cursor:pointer;margin-right:.75rem;}
.mask{display:none;position:fixed;top:3.5rem;left:0;right:0;bottom:0;background:rgba(0,0,0,.5);z-index:99;}
.mask.active{display:block;}
</style>
</head>
<body>

<!-- Toast container -->
<div class="toast-container" id="toastContainer"></div>
<div class="mask" id="mask" onclick="closeSidebar()"></div>

<!-- Top bar -->
<div class="topbar">
  <button class="menu-btn" onclick="toggleSidebar()">&#9776;</button>
  <span class="logo-text">&#127781; WeatherEPD</span>
  <span class="logo-sub">Config Portal</span>
</div>

<!-- Sidebar -->
<div class="sidebar" id="sidebar">
  <ul class="menu">
    <li><a onclick="nav('settings')" id="nav-settings" class="active">
      <span class="menu-icon">&#9881;&#65039;</span>Settings</a></li>
    <li><a onclick="nav('logs')" id="nav-logs">
      <span class="menu-icon">&#128196;</span>Logs</a></li>
  </ul>
</div>

<!-- Main -->
<div class="main">

  <!-- ===== Settings page ===== -->
  <div id="page-settings" class="page active">

    <!-- WiFi Card -->
    <div class="card">
      <h2 class="card-title"><span class="card-icon">&#128225;</span>Network &amp; WiFi</h2>
      <div class="card-body">
        <div class="field-grid">
          <span class="field-label">WiFi SSID</span>
          <input class="field-input" type="text" id="s_WifiSSID" placeholder="Your WiFi name">
        </div>
        <div class="field-grid">
          <span class="field-label">WiFi Password</span>
          <input class="field-input" type="password" id="s_WifiPSWD" placeholder="••••••••">
        </div>
      </div>
      <div class="card-footer"><button class="btn-save" onclick="saveCard(['WifiSSID','WifiPSWD'])">Save</button></div>
    </div>

    <!-- Location Card -->
    <div class="card">
      <h2 class="card-title"><span class="card-icon">&#127759;</span>Location</h2>
      <div class="card-body">
        <div class="field-grid">
          <span class="field-label">City Name</span>
          <input class="field-input" type="text" id="s_City" placeholder="e.g. Beijing">
        </div>
        <div class="field-grid">
          <span class="field-label">Latitude</span>
          <input class="field-input" type="text" id="s_Lat" placeholder="e.g. 39.9042">
        </div>
        <div class="field-grid">
          <span class="field-label">Longitude</span>
          <input class="field-input" type="text" id="s_Lon" placeholder="e.g. 116.4074">
        </div>
        <p class="field-hint">Coordinates used for Open-Meteo weather API (no API key required)</p>
      </div>
      <div class="card-footer"><button class="btn-save" onclick="saveCard(['City','Lat','Lon'])">Save</button></div>
    </div>

    <!-- Time Card -->
    <div class="card">
      <h2 class="card-title"><span class="card-icon">&#128336;</span>Time &amp; Schedule</h2>
      <div class="card-body">
        <div class="field-grid">
          <span class="field-label">Timezone</span>
          <input class="field-input" type="text" id="s_Timezone" placeholder="e.g. CST-8 or EST5EDT,M3.2.0,M11.1.0">
        </div>
        <div class="field-hint">POSIX TZ string — <a href="https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html" target="_blank">reference</a></div>
        <div class="field-grid" style="margin-top:.5rem;">
          <span class="field-label">Refresh Interval</span>
          <input class="field-input" type="number" id="s_SleepMin" min="5" max="1440" placeholder="minutes">
        </div>
        <div class="field-grid">
          <span class="field-label">Bed Time (h)</span>
          <input class="field-input" type="number" id="s_BedTime" min="0" max="23" placeholder="0-23, pause updates">
        </div>
        <div class="field-grid">
          <span class="field-label">Wake Time (h)</span>
          <input class="field-input" type="number" id="s_WakeTime" min="0" max="23" placeholder="0-23, resume updates">
        </div>
      </div>
      <div class="card-footer"><button class="btn-save" onclick="saveCard(['Timezone','SleepMin','BedTime','WakeTime'])">Save</button></div>
    </div>

    <!-- Units Card -->
    <div class="card">
      <h2 class="card-title"><span class="card-icon">&#128207;</span>Units</h2>
      <div class="card-body">
        <div class="field-grid">
          <span class="field-label">Temperature</span>
          <select class="field-select" id="s_UnitsTemp">
            <option value="C">°C (Celsius)</option>
            <option value="F">°F (Fahrenheit)</option>
          </select>
        </div>
        <div class="field-grid">
          <span class="field-label">Wind Speed</span>
          <select class="field-select" id="s_UnitsSpeed">
            <option value="kmh">km/h</option>
            <option value="ms">m/s</option>
            <option value="mph">mph</option>
            <option value="kn">knots</option>
          </select>
        </div>
        <div class="field-grid">
          <span class="field-label">Pressure</span>
          <select class="field-select" id="s_UnitsPres">
            <option value="hPa">hPa</option>
            <option value="inHg">inHg</option>
            <option value="mmHg">mmHg</option>
          </select>
        </div>
        <div class="field-grid">
          <span class="field-label">Distance</span>
          <select class="field-select" id="s_UnitsDist">
            <option value="km">km</option>
            <option value="mi">miles</option>
          </select>
        </div>
        <div class="field-grid">
          <span class="field-label">Precipitation</span>
          <select class="field-select" id="s_UnitsPrecip">
            <option value="mm">mm</option>
            <option value="in">inches</option>
          </select>
        </div>
      </div>
      <div class="card-footer"><button class="btn-save" onclick="saveCard(['UnitsTemp','UnitsSpeed','UnitsPres','UnitsDist','UnitsPrecip'])">Save</button></div>
    </div>

    <!-- Display Card -->
    <div class="card">
      <h2 class="card-title"><span class="card-icon">&#128250;</span>Display &amp; Locale</h2>
      <div class="card-body">
        <div class="field-grid">
          <span class="field-label">Language</span>
          <select class="field-select" id="s_Language">
            <option value="en_US">English (en_US)</option>
            <option value="zh_CN">中文 (zh_CN)</option>
          </select>
        </div>
        <div class="field-grid">
          <span class="field-label">Time Format</span>
          <input class="field-input" type="text" id="s_TimeFmt" placeholder="e.g. %H:%M">
        </div>
        <div class="field-grid">
          <span class="field-label">Date Format</span>
          <input class="field-input" type="text" id="s_DateFmt" placeholder="e.g. %a, %B %e">
        </div>
        <p class="field-hint">strftime format strings — <a href="https://cppreference.com/w/c/chrono/strftime" target="_blank">reference</a></p>
      </div>
      <div class="card-footer"><button class="btn-save" onclick="saveCard(['Language','TimeFmt','DateFmt'])">Save</button></div>
    </div>

  </div><!-- /page-settings -->

  <!-- ===== Logs page ===== -->
  <div id="page-logs" class="page">
    <div class="placeholder">
      <div class="ph-icon">&#128196;</div>
      <div class="ph-title">Logs</div>
      <div class="ph-desc">Coming soon — serial log viewer over WebSocket</div>
    </div>
  </div>

</div><!-- /main -->

<script>
// ── Navigation ──────────────────────────────────────────────────────────────
function nav(page){
  document.querySelectorAll('.page').forEach(function(p){p.classList.remove('active');});
  document.querySelectorAll('.menu li a').forEach(function(a){a.classList.remove('active');});
  document.getElementById('page-'+page).classList.add('active');
  document.getElementById('nav-'+page).classList.add('active');
  closeSidebar();
}
function toggleSidebar(){
  document.getElementById('sidebar').classList.toggle('open');
  document.getElementById('mask').classList.toggle('active');
}
function closeSidebar(){
  document.getElementById('sidebar').classList.remove('open');
  document.getElementById('mask').classList.remove('active');
}

// ── Toast ────────────────────────────────────────────────────────────────────
function showToast(title, msg, type){
  type = type||'info';
  var c=document.getElementById('toastContainer');
  var t=document.createElement('div');
  t.className='toast '+type;
  t.innerHTML='<div class="toast-title">'+title+'</div>'+(msg?'<div class="toast-msg">'+msg+'</div>':'');
  c.appendChild(t);
  requestAnimationFrame(function(){t.classList.add('in');});
  setTimeout(function(){
    t.classList.remove('in');t.classList.add('out');
    setTimeout(function(){c.removeChild(t);},350);
  },3500);
}

// ── Config load / save ───────────────────────────────────────────────────────
var _cfg = {};

function loadConfig(){
  fetch('/api/config')
    .then(function(r){return r.json();})
    .then(function(d){
      _cfg = d;
      var keys = Object.keys(d);
      keys.forEach(function(k){
        var el = document.getElementById('s_'+k);
        if(!el) return;
        el.value = d[k];
      });
    })
    .catch(function(e){showToast('Load failed', String(e), 'error');});
}

function saveCard(keys){
  var patch = {};
  var valid = true;
  keys.forEach(function(k){
    var el = document.getElementById('s_'+k);
    if(!el) return;
    var v = el.value.trim();
    if(v === '' && ['WifiSSID','Lat','Lon','City','Timezone'].indexOf(k) >= 0){
      showToast('Validation', k+' cannot be empty', 'error');
      valid = false;
    }
    patch[k] = v;
  });
  if(!valid) return;

  fetch('/api/config', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(patch)
  })
  .then(function(r){
    if(r.ok){ showToast('Saved', 'Settings saved to NVS', 'success'); }
    else{ return r.text().then(function(t){showToast('Save failed', t, 'error');}); }
  })
  .catch(function(e){showToast('Save failed', String(e), 'error');});
}

// ── Init ─────────────────────────────────────────────────────────────────────
loadConfig();
</script>
</body>
</html>
)HTMLEOF";
