#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ClipSampler10 - Generador de Licencias, Base de Datos SQLite y Servidor de Activación API
=========================================================================================
Características:
1. Base de Datos SQLite persistente (`licenses.db`) que almacena todas las licencias emitidas,
   datos del cliente, ID de Máquina (Hardware ID), estado (ACTIVA / REVOCADA) y fecha de activación.
2. Algoritmo criptográfico idéntico al motor C++ (`Source/Security/LicenseManager.cpp`):
   - Soporta Licencias Directas por ID de Máquina (`CS10-<HW4>-<SER4>-<SIG4>-<SIG4>`) que
     funcionan tanto Online como 100% Offline.
   - Soporta Licencias Automáticas para Página Web (`CS10-AUTO-<SER4>-<TOK4>-<TOK4>`) que se
     auto-vinculan a la primera computadora que las activa vía `/api/activate` y bloquean cualquier otra PC.
3. Webhook `/api/webhook/generate` listo para integrarse con tu futura página web / pasarela de pagos
   para generar y enviar licencias automáticamente tras cada compra.
4. Panel de Administración Web moderno en `http://127.0.0.1:8765`.
"""

import hashlib
import json
import os
import secrets
import sqlite3
import sys
import threading
import webbrowser
from datetime import datetime
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

# Clave maestra idéntica a kMasterSigningSecret en Source/Security/LicenseManager.cpp
MASTER_SIGNING_SECRET = "CS10_MASTER_SIG_2026_AUDIOINNOVATIONS_9F8E7D6C5B4A392817263544"

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.path.join(BASE_DIR, "licenses.db")
PORT = 8765


def compute_cpp_license_signature(machine_id: str, tier_code: str, serial_block: str) -> str:
    """Replica exactamente LicenseManager::computeLicenseSignature del plugin C++."""
    clean_mch = machine_id.strip().upper()
    clean_tier = tier_code.strip().upper()
    clean_serial = serial_block.strip().upper()

    payload = f"{clean_mch}|{clean_tier}|{clean_serial}|{MASTER_SIGNING_SECRET}"
    first_pass = hashlib.sha256(payload.encode("utf-8")).hexdigest().upper()

    second_payload = f"{first_pass}|{MASTER_SIGNING_SECRET}"
    second_pass = hashlib.sha256(second_payload.encode("utf-8")).hexdigest().upper()
    return second_pass[:8]


def generate_machine_bound_key(machine_id: str, serial_block: str = "") -> str:
    """Genera una clave CS10-<HW4>-<SER4>-<SIG4>-<SIG4> atada criptográficamente a un Machine ID."""
    clean_mch = machine_id.strip().upper()
    parts = [p for p in clean_mch.split("-") if p]
    if len(parts) < 2 or parts[0] != "MCH" or len(parts[1]) != 4:
        raise ValueError("El ID de Máquina debe tener el formato MCH-XXXX-XXXX-XXXX")

    hw_tag = parts[1][:4]
    if not serial_block:
        serial_block = secrets.token_hex(2).upper()
    else:
        serial_block = serial_block.strip().upper()[:4].ljust(4, "0")

    sig8 = compute_cpp_license_signature(clean_mch, hw_tag, serial_block)
    return f"CS10-{hw_tag}-{serial_block}-{sig8[:4]}-{sig8[4:8]}"


def generate_auto_web_key() -> str:
    """Genera una clave CS10-AUTO-XXXX-XXXX-XXXX para venta automática en página web."""
    b1 = secrets.token_hex(2).upper()
    b2 = secrets.token_hex(2).upper()
    b3 = secrets.token_hex(2).upper()
    return f"CS10-AUTO-{b1}-{b2}-{b3}"


def init_db():
    conn = sqlite3.connect(DB_PATH)
    cur = conn.cursor()
    cur.execute(
        """
        CREATE TABLE IF NOT EXISTS licenses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            license_key TEXT UNIQUE NOT NULL,
            bound_license_key TEXT DEFAULT '',
            license_mode TEXT NOT NULL,
            client_name TEXT NOT NULL,
            client_email TEXT DEFAULT '',
            machine_id TEXT DEFAULT '',
            status TEXT DEFAULT 'ACTIVE',
            activations_count INTEGER DEFAULT 0,
            created_at TEXT NOT NULL,
            activated_at TEXT DEFAULT '',
            notes TEXT DEFAULT ''
        )
        """
    )
    conn.commit()
    conn.close()


DASHBOARD_HTML = """<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ClipSampler10 — Generador y Base de Datos de Licencias</title>
<style>
  :root {
    --bg: #0f1218;
    --card: #181c26;
    --border: #2a3142;
    --cyan: #00e5ff;
    --pink: #ff0055;
    --green: #00e676;
    --amber: #ffb74d;
    --text: #e4e9f2;
    --muted: #8c9ba5;
  }
  * { box-sizing: border-box; font-family: 'Segoe UI', system-ui, sans-serif; }
  body { margin: 0; background: var(--bg); color: var(--text); padding: 24px; }
  .container { max-width: 1220px; margin: 0 auto; }
  header {
    display: flex; justify-content: space-between; align-items: center;
    background: var(--card); padding: 18px 24px; border-radius: 12px;
    border: 1px solid var(--border); margin-bottom: 20px;
  }
  h1 { margin: 0; font-size: 22px; color: var(--cyan); letter-spacing: 0.5px; }
  .subtitle { font-size: 13px; color: var(--muted); margin-top: 4px; }
  .stats { display: flex; gap: 14px; }
  .stat-box {
    background: #12151e; border: 1px solid var(--border); border-radius: 8px;
    padding: 10px 16px; text-align: center; min-width: 110px;
  }
  .stat-num { font-size: 20px; font-weight: 700; color: var(--cyan); }
  .stat-label { font-size: 11px; color: var(--muted); text-transform: uppercase; }
  .grid { display: grid; grid-template-columns: 380px 1fr; gap: 20px; }
  .card {
    background: var(--card); border: 1px solid var(--border);
    border-radius: 12px; padding: 20px;
  }
  .card h2 { margin-top: 0; font-size: 16px; color: #fff; border-bottom: 1px solid var(--border); padding-bottom: 10px; }
  label { display: block; font-size: 12px; font-weight: 600; color: var(--muted); margin-bottom: 6px; margin-top: 12px; }
  input, select, textarea {
    width: 100%; padding: 10px 12px; background: #12151e; border: 1px solid var(--border);
    border-radius: 6px; color: #fff; font-size: 13.5px;
  }
  input:focus, select:focus, textarea:focus { outline: none; border-color: var(--cyan); }
  .btn {
    display: inline-block; width: 100%; padding: 12px; margin-top: 16px;
    background: var(--pink); color: #fff; border: none; border-radius: 6px;
    font-weight: 700; font-size: 14px; cursor: pointer; transition: opacity 0.15s;
  }
  .btn:hover { opacity: 0.9; }
  .btn-sm {
    padding: 5px 10px; font-size: 11.5px; border-radius: 5px; border: 1px solid var(--border);
    background: #222836; color: #fff; cursor: pointer; margin-right: 4px;
  }
  .btn-sm:hover { border-color: var(--cyan); }
  .btn-danger { background: #4a1525; border-color: #ff1744; color: #ff8a80; }
  .btn-success { background: #0f3d29; border-color: #00e676; color: #00e676; }
  .generated-box {
    margin-top: 16px; padding: 14px; border-radius: 8px; background: #101f24;
    border: 1px solid var(--cyan); display: none; text-align: center;
  }
  .generated-key { font-size: 17px; font-weight: 800; color: var(--cyan); letter-spacing: 1px; margin: 8px 0; word-break: break-all; }
  table { width: 100%; border-collapse: collapse; margin-top: 12px; font-size: 13px; }
  th, td { padding: 10px 8px; text-align: left; border-bottom: 1px solid var(--border); }
  th { color: var(--muted); font-size: 11.5px; text-transform: uppercase; }
  .badge {
    padding: 3px 8px; border-radius: 4px; font-size: 11px; font-weight: 700;
  }
  .badge-active { background: #0f3d29; color: var(--green); }
  .badge-revoked { background: #4a121f; color: #ff5252; }
  .badge-mode { background: #1e2638; color: var(--cyan); }
  .code-pill {
    font-family: Consolas, monospace; font-weight: 700; color: var(--cyan);
    background: #12151e; padding: 4px 7px; border-radius: 4px; border: 1px solid #252c3d;
  }
  .toolbar { display: flex; justify-content: space-between; gap: 10px; align-items: center; }
</style>
</head>
<body>
<div class="container">
  <header>
    <div>
      <h1>🔐 CLIPSAMPLER 10 — PANEL DE LICENCIAS Y BASE DE DATOS</h1>
      <div class="subtitle">Generador Criptográfico + Servidor de Activación Online & Webhook para Página Web (Base de datos SQLite activa)</div>
    </div>
    <div class="stats">
      <div class="stat-box"><div class="stat-num" id="statTotal">0</div><div class="stat-label">Total</div></div>
      <div class="stat-box"><div class="stat-num" id="statActive" style="color:var(--green)">0</div><div class="stat-label">Activas</div></div>
      <div class="stat-box"><div class="stat-num" id="statBound" style="color:var(--amber)">0</div><div class="stat-label">Vinculadas a PC</div></div>
    </div>
  </header>

  <div class="grid">
    <!-- Panel Izquierdo: Generador -->
    <div class="card">
      <h2>➕ Generar Nueva Licencia</h2>
      <label>Modo de Protección de Licencia</label>
      <select id="licenseMode" onchange="toggleMachineInput()">
        <option value="MACHINE_BOUND">🔒 Atada a ID de Máquina (Funciona Offline y Online en 1 sola PC)</option>
        <option value="AUTO_WEB">🌐 Auto-Vinculable Web (Se ata automáticamente a la 1ª PC que la active)</option>
      </select>

      <div id="machineIdGroup">
        <label>ID de Máquina del Cliente (MCH-XXXX-XXXX-XXXX)</label>
        <input type="text" id="machineId" placeholder="Ej: MCH-8F3A-92C1-4B0E" style="font-family:Consolas,monospace;color:var(--cyan);font-weight:bold;">
      </div>

      <label>Nombre del Cliente</label>
      <input type="text" id="clientName" placeholder="Ej: Juan Pérez / Estudio Musical">

      <label>Correo Electrónico / Contacto</label>
      <input type="email" id="clientEmail" placeholder="cliente@correo.com">

      <label>Notas (Opcional)</label>
      <textarea id="notes" rows="2" placeholder="Orden #1042, Licencia Pro Perpetua..."></textarea>

      <button class="btn" onclick="createLicense()">⚡ GENERAR Y GUARDAR EN BASE DE DATOS</button>

      <div class="generated-box" id="generatedBox">
        <div style="font-size:12px;color:var(--muted);">¡Licencia generada y guardada en <code>licenses.db</code>!</div>
        <div class="generated-key" id="generatedKeyText"></div>
        <button class="btn-sm btn-success" onclick="copyGeneratedKey()">📋 Copiar Código de Licencia</button>
      </div>
    </div>

    <!-- Panel Derecho: Base de Datos -->
    <div class="card">
      <div class="toolbar">
        <h2 style="border:none;margin:0;padding:0;">🗄️ Licencias Registradas en Base de Datos</h2>
        <div style="display:flex;gap:8px;width:340px;">
          <input type="text" id="searchInput" placeholder="Buscar cliente, correo, licencia o ID..." oninput="renderTable()" style="margin:0;">
          <button class="btn-sm" onclick="exportCsv()" style="white-space:nowrap;">⬇ Exportar CSV</button>
        </div>
      </div>

      <div style="overflow-x:auto;">
        <table>
          <thead>
            <tr>
              <th>Código de Licencia</th>
              <th>Cliente</th>
              <th>Modo</th>
              <th>ID de Máquina (PC)</th>
              <th>Estado</th>
              <th>Fecha</th>
              <th>Acciones</th>
            </tr>
          </thead>
          <tbody id="licensesTableBody"></tbody>
        </table>
      </div>
    </div>
  </div>
</div>

<script>
let allLicenses = [];

function toggleMachineInput() {
  const mode = document.getElementById('licenseMode').value;
  document.getElementById('machineIdGroup').style.display = (mode === 'MACHINE_BOUND') ? 'block' : 'none';
}

async function fetchLicenses() {
  const res = await fetch('/api/licenses');
  const data = await res.json();
  allLicenses = data.licenses || [];
  document.getElementById('statTotal').textContent = allLicenses.length;
  document.getElementById('statActive').textContent = allLicenses.filter(l => l.status === 'ACTIVE').length;
  document.getElementById('statBound').textContent = allLicenses.filter(l => l.machine_id && l.machine_id.length > 0).length;
  renderTable();
}

function renderTable() {
  const q = document.getElementById('searchInput').value.toLowerCase();
  const tbody = document.getElementById('licensesTableBody');
  tbody.innerHTML = '';

  const filtered = allLicenses.filter(l =>
    l.license_key.toLowerCase().includes(q) ||
    l.client_name.toLowerCase().includes(q) ||
    (l.client_email || '').toLowerCase().includes(q) ||
    (l.machine_id || '').toLowerCase().includes(q)
  );

  for (const lic of filtered) {
    const tr = document.createElement('tr');
    const isRevoked = lic.status === 'REVOKED';
    tr.innerHTML = `
      <td><span class="code-pill">${lic.license_key}</span></td>
      <td><strong>${lic.client_name}</strong><br><span style="font-size:11px;color:var(--muted)">${lic.client_email || ''}</span></td>
      <td><span class="badge badge-mode">${lic.license_mode === 'AUTO_WEB' ? 'WEB AUTO' : 'HW LOCKED'}</span></td>
      <td style="font-family:Consolas,monospace;font-size:12px;color:${lic.machine_id ? '#ffb74d' : 'var(--muted)'}">
        ${lic.machine_id || 'Sin vincular aún'}
      </td>
      <td><span class="badge ${isRevoked ? 'badge-revoked' : 'badge-active'}">${lic.status}</span></td>
      <td style="font-size:11.5px;color:var(--muted)">${lic.created_at}</td>
      <td>
        <button class="btn-sm" onclick="copyText('${lic.license_key}')">Copiar</button>
        ${lic.license_mode === 'AUTO_WEB' && lic.machine_id ? `<button class="btn-sm" onclick="resetMachine(${lic.id})">Liberar PC</button>` : ''}
        <button class="btn-sm ${isRevoked ? 'btn-success' : 'btn-danger'}" onclick="toggleStatus(${lic.id}, '${isRevoked ? 'ACTIVE' : 'REVOKED'}')">
          ${isRevoked ? 'Reactivar' : 'Revocar'}
        </button>
      </td>
    `;
    tbody.appendChild(tr);
  }
}

async function createLicense() {
  const mode = document.getElementById('licenseMode').value;
  const machineId = document.getElementById('machineId').value.trim();
  const clientName = document.getElementById('clientName').value.trim() || 'Cliente General';
  const clientEmail = document.getElementById('clientEmail').value.trim();
  const notes = document.getElementById('notes').value.trim();

  const res = await fetch('/api/generate', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ license_mode: mode, machine_id: machineId, client_name: clientName, client_email: clientEmail, notes })
  });
  const data = await res.json();
  if (!data.success) {
    alert(data.message || 'Error al generar licencia');
    return;
  }

  const box = document.getElementById('generatedBox');
  document.getElementById('generatedKeyText').textContent = data.license_key;
  box.style.display = 'block';
  fetchLicenses();
}

function copyGeneratedKey() {
  const key = document.getElementById('generatedKeyText').textContent;
  copyText(key);
}

function copyText(txt) {
  navigator.clipboard.writeText(txt);
}

async function toggleStatus(id, newStatus) {
  await fetch('/api/status', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ id, status: newStatus })
  });
  fetchLicenses();
}

async function resetMachine(id) {
  if (!confirm('¿Liberar el ID de Máquina de esta licencia para permitir que el cliente la active en una nueva PC?')) return;
  await fetch('/api/reset_machine', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ id })
  });
  fetchLicenses();
}

function exportCsv() {
  window.location.href = '/api/export_csv';
}

fetchLicenses();
</script>
</body>
</html>
"""


class LicenseRequestHandler(BaseHTTPRequestHandler):
    def _send_json(self, payload: dict, status: int = 200):
        raw = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def _read_json_body(self) -> dict:
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == "/" or parsed.path == "/index.html":
            raw = DASHBOARD_HTML.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(raw)))
            self.end_headers()
            self.wfile.write(raw)
            return

        if parsed.path == "/api/licenses":
            conn = sqlite3.connect(DB_PATH)
            conn.row_factory = sqlite3.Row
            rows = conn.execute("SELECT * FROM licenses ORDER BY id DESC").fetchall()
            conn.close()
            self._send_json({"success": True, "licenses": [dict(r) for r in rows]})
            return

        if parsed.path == "/api/export_csv":
            conn = sqlite3.connect(DB_PATH)
            conn.row_factory = sqlite3.Row
            rows = conn.execute("SELECT * FROM licenses ORDER BY id DESC").fetchall()
            conn.close()
            lines = ["id,license_key,bound_license_key,license_mode,client_name,client_email,machine_id,status,activations_count,created_at,activated_at,notes"]
            for r in rows:
                lines.append(
                    f'{r["id"]},"{r["license_key"]}","{r["bound_license_key"]}","{r["license_mode"]}","{r["client_name"]}","{r["client_email"]}","{r["machine_id"]}","{r["status"]}",{r["activations_count"]},"{r["created_at"]}","{r["activated_at"]}","{r["notes"]}"'
                )
            csv_bytes = "\n".join(lines).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/csv; charset=utf-8")
            self.send_header("Content-Disposition", 'attachment; filename="ClipSampler10_Licenses.csv"')
            self.send_header("Content-Length", str(len(csv_bytes)))
            self.end_headers()
            self.wfile.write(csv_bytes)
            return

        self._send_json({"success": False, "message": "Ruta no encontrada"}, 404)

    def do_POST(self):
        parsed = urlparse(self.path)
        body = self._read_json_body()

        # 1. Generar licencia desde el Dashboard o desde un Webhook de página web
        if parsed.path in ("/api/generate", "/api/webhook/generate"):
            mode = body.get("license_mode", "AUTO_WEB").strip().upper()
            machine_id = body.get("machine_id", "").strip().upper()
            client_name = body.get("client_name", "Cliente Web").strip()
            client_email = body.get("client_email", "").strip()
            notes = body.get("notes", "").strip()
            now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            try:
                if mode == "MACHINE_BOUND":
                    lic_key = generate_machine_bound_key(machine_id)
                    bound_key = lic_key
                else:
                    mode = "AUTO_WEB"
                    lic_key = generate_auto_web_key()
                    bound_key = ""
                    machine_id = ""
            except ValueError as err:
                self._send_json({"success": False, "message": str(err)}, 400)
                return

            conn = sqlite3.connect(DB_PATH)
            cur = conn.cursor()
            cur.execute(
                """
                INSERT INTO licenses (license_key, bound_license_key, license_mode, client_name, client_email, machine_id, status, created_at, notes)
                VALUES (?, ?, ?, ?, ?, ?, 'ACTIVE', ?, ?)
                """,
                (lic_key, bound_key, mode, client_name, client_email, machine_id, now_str, notes),
            )
            conn.commit()
            conn.close()

            self._send_json({"success": True, "license_key": lic_key, "license_mode": mode})
            return

        # 2. Endpoint de Activación invocado por el plugin C++ ClipSampler10
        if parsed.path == "/api/activate":
            lic_key = body.get("license_key", "").strip().upper()
            machine_id = body.get("machine_id", "").strip().upper()
            now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            if not lic_key or not machine_id:
                self._send_json({"success": False, "message": "Faltan parámetros license_key o machine_id."}, 400)
                return

            conn = sqlite3.connect(DB_PATH)
            conn.row_factory = sqlite3.Row
            row = conn.execute(
                "SELECT * FROM licenses WHERE license_key = ? OR bound_license_key = ?",
                (lic_key, lic_key),
            ).fetchone()

            if row is None:
                conn.close()
                self._send_json({"success": False, "message": "La licencia ingresada no existe en la base de datos."}, 404)
                return

            if row["status"] != "ACTIVE":
                conn.close()
                self._send_json({"success": False, "message": "Esta licencia ha sido revocada o suspendida por el administrador."}, 403)
                return

            existing_mch = (row["machine_id"] or "").strip().upper()
            if existing_mch and existing_mch != machine_id:
                conn.close()
                self._send_json(
                    {
                        "success": False,
                        "message": f"Piratería bloqueada: Esta licencia ya está vinculada a otro equipo ({existing_mch}).",
                    },
                    403,
                )
                return

            # Si es la primera activación de una clave AUTO_WEB, firmamos criptográficamente para su Machine ID
            try:
                serial_part = row["license_key"].split("-")[2]
                bound_key = generate_machine_bound_key(machine_id, serial_part)
            except Exception as exc:
                conn.close()
                self._send_json({"success": False, "message": f"ID de máquina inválido: {exc}"}, 400)
                return

            conn.execute(
                """
                UPDATE licenses
                SET machine_id = ?, bound_license_key = ?, activations_count = activations_count + 1,
                    activated_at = CASE WHEN activated_at = '' THEN ? ELSE activated_at END
                WHERE id = ?
                """,
                (machine_id, bound_key, now_str, row["id"]),
            )
            conn.commit()
            conn.close()

            self._send_json(
                {
                    "success": True,
                    "bound_license_key": bound_key,
                    "message": "Licencia activada y vinculada oficialmente a este equipo.",
                }
            )
            return

        # 3. Cambiar estado ACTIVE / REVOKED
        if parsed.path == "/api/status":
            lic_id = int(body.get("id", 0))
            new_status = body.get("status", "ACTIVE").strip().upper()
            conn = sqlite3.connect(DB_PATH)
            conn.execute("UPDATE licenses SET status = ? WHERE id = ?", (new_status, lic_id))
            conn.commit()
            conn.close()
            self._send_json({"success": True})
            return

        # 4. Liberar Machine ID para permitir cambio de PC a un cliente legítimo
        if parsed.path == "/api/reset_machine":
            lic_id = int(body.get("id", 0))
            conn = sqlite3.connect(DB_PATH)
            conn.execute("UPDATE licenses SET machine_id = '', bound_license_key = '' WHERE id = ?", (lic_id,))
            conn.commit()
            conn.close()
            self._send_json({"success": True})
            return

        self._send_json({"success": False, "message": "Endpoint no encontrado"}, 404)

    def log_message(self, fmt, *args):
        sys.stdout.write(f"[LicenseServer] {self.address_string()} - {fmt % args}\n")


def main():
    init_db()
    server = HTTPServer(("127.0.0.1", PORT), LicenseRequestHandler)
    url = f"http://127.0.0.1:{PORT}"
    print("=" * 68)
    print("  ClipSampler10 - Generador de Licencias y Base de Datos Activo")
    print(f"  Base de datos SQLite : {DB_PATH}")
    print(f"  Panel de Control Web : {url}")
    print(f"  API de Activacion    : {url}/api/activate")
    print(f"  Webhook Pagina Web   : {url}/api/webhook/generate")
    print("=" * 68)

    if "--no-browser" not in sys.argv:
        threading.Timer(0.8, lambda: webbrowser.open(url)).start()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nCerrando servidor de licencias...")
        server.server_close()


if __name__ == "__main__":
    main()
