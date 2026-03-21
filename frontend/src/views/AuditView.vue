<template>
  <div>
    <h2><i class="bi bi-journal-text"></i> Журнал аудита</h2>

    <div class="row g-2 mb-3 align-items-end">
      <div class="col-md-3">
        <label class="form-label">Тип объекта</label>
        <select v-model="objectType" class="form-select form-select-sm" @change="load">
          <option value="">Все</option>
          <option v-for="t in objectTypes" :key="t" :value="t">{{ t }}</option>
        </select>
      </div>
    </div>

    <div v-if="logs.length" class="table-responsive">
      <table class="table table-sm table-hover">
        <thead class="table-light">
          <tr><th>Дата/время</th><th>Пользователь</th><th>Действие</th><th>Тип</th><th>ID</th><th>Было</th><th>Стало</th><th>Контекст</th><th>IP</th></tr>
        </thead>
        <tbody>
          <tr v-for="log in logs" :key="log.id">
            <td>{{ formatDate(log.created_at) }}</td>
            <td>{{ log.user || '—' }}</td>
            <td><span class="badge bg-secondary">{{ log.action }}</span></td>
            <td>{{ log.object_type }}</td>
            <td><code>{{ log.object_id || '—' }}</code></td>
            <td>{{ (log.old_value || '—').slice(0, 30) }}</td>
            <td>{{ (log.new_value || '—').slice(0, 30) }}</td>
            <td>{{ log.context || '—' }}</td>
            <td><small>{{ log.ip_address || '—' }}</small></td>
          </tr>
        </tbody>
      </table>
    </div>
    <div v-else class="text-center py-5"><p class="text-muted">Записей нет</p></div>

    <nav v-if="totalPages > 1">
      <ul class="pagination pagination-sm justify-content-center">
        <li v-for="p in totalPages" :key="p" :class="{ 'page-item': true, active: p === page }">
          <a class="page-link" href="#" @click.prevent="page = p; load()">{{ p }}</a>
        </li>
      </ul>
    </nav>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import api from '../api/client'

const logs = ref([])
const page = ref(1)
const totalPages = ref(1)
const objectType = ref('')
const objectTypes = ['requirement','trace_link','project','user','change_request','baseline','auth','notification']

function formatDate(iso) {
    return iso ? new Date(iso).toLocaleString('ru-RU', { day:'2-digit', month:'2-digit', year:'numeric', hour:'2-digit', minute:'2-digit', second:'2-digit' }) : '—'
}

async function load() {
    const params = { page: page.value }
    if (objectType.value) params.object_type = objectType.value
    const { data } = await api.get('/audit', { params })
    logs.value = data.items
    totalPages.value = data.pages
}

onMounted(load)
</script>
