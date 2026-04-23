/* === Navigation === */
function switchPage(pageId, el) {
  document.querySelectorAll('.page').forEach(function(p) { p.classList.remove('active'); });
  document.getElementById('page-' + pageId).classList.add('active');
  document.querySelectorAll('.nav-link').forEach(function(a) { a.classList.remove('active'); });
  if (el) el.classList.add('active');
  closeSidebar();
  if (pageId === 'settings' && !settingsLoaded) loadSettingsConfig();
}

function toggleSidebar() {
  document.getElementById('sidebar').classList.toggle('open');
  document.getElementById('layoutMask').classList.toggle('active');
}

function closeSidebar() {
  if (window.innerWidth <= 991) {
    document.getElementById('sidebar').classList.remove('open');
    document.getElementById('layoutMask').classList.remove('active');
  }
}

/* === Toast === */
function showToast(msg, isError, type, title) {
  var toastType = type || (isError ? 'error' : 'success');
  var toastTitle = title || (isError ? 'Error' : 'Saved');
  var container = document.getElementById('toast-container');
  var el = document.createElement('div');
  el.className = 'ngx-toastr toast-' + toastType;
  el.innerHTML =
    '<button class="toast-close-btn" onclick="var p=this.parentNode;p.classList.add(\'toast-out\');setTimeout(function(){if(p.parentNode)p.parentNode.removeChild(p);},350);">&#x2715;</button>' +
    '<div class="toast-title">' + toastTitle + '</div>' +
    '<div class="toast-detail">' + msg + '</div>' +
    '<div class="toast-progress" style="width:100%;"></div>';
  container.appendChild(el);
  requestAnimationFrame(function() {
    requestAnimationFrame(function() { el.classList.add('toast-in'); });
  });
  var prog = el.querySelector('.toast-progress');
  var duration = 4000;
  var start = null;
  function anim(ts) {
    if (!start) start = ts;
    var pct = 100 - Math.min(100, (ts - start) / duration * 100);
    prog.style.width = pct + '%';
    if (pct > 0 && el.parentNode) requestAnimationFrame(anim);
  }
  requestAnimationFrame(anim);
  el._tid = setTimeout(function() {
    el.classList.add('toast-out');
    setTimeout(function() { if (el.parentNode) el.parentNode.removeChild(el); }, 350);
  }, duration);
}

/* === Device Restart === */
function restartLocalDevice() {
  if (!confirm('Restart the weather display?')) return;
  fetch('/api/system/restart', { method: 'POST', headers: { 'Content-Type': 'application/json' } })
    .then(function(r) {
      if (r.ok) showToast('Device is restarting...', false, 'info', 'Restart');
      else showToast('Restart request failed', true);
    })
    .catch(function() { showToast('Device is restarting...', false, 'info', 'Restart'); });
}

/* === Init on load: settings is default page === */
(function() {
  function init() {
    var active = document.querySelector('.page.active');
    if (active && active.id === 'page-settings') loadSettingsConfig();
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init);
  else init();
})();
