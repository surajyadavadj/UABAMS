# React App (Development Handoff / Machine-to-Machine)

This repo is meant to be cloned/pulled and run in **development mode** using **npm**.
Do **not** copy `node_modules/` or build outputs across machines.

---

## What gets copied / committed
Keep in Git / include in ZIP:
- `src/`
- `public/` (or framework equivalent)
- `package.json`
- `package-lock.json` (required for reproducible installs)
- Config files (`vite.config.*`, `webpack.config.*`, `tsconfig.json`, etc.)
- `.env.example` (sample env vars, no secrets)

Do NOT include:
- `node_modules/`
- `dist/`, `build/`, `.next/`
- `.env` (real secrets)
- cache folders, logs

---

## Step-by-step: move the project to another machine (ZIP method)

### On the source machine
1. **Verify the project runs**
   ```bash
   npm install
   npm run dev
