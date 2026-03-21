<template>
  <div>
    <nav aria-label="breadcrumb">
      <ol class="breadcrumb">
        <li class="breadcrumb-item"><router-link to="/">Проекты</router-link></li>
        <li class="breadcrumb-item active">{{ project?.name }}</li>
      </ol>
    </nav>

    <div class="d-flex justify-content-between align-items-center mb-3">
      <h2><i class="bi bi-list-check"></i> Требования</h2>
      <div class="btn-group">
        <router-link v-if="auth.can('create')" :to="`/projects/${projectId}/requirements/new`" class="btn btn-primary btn-sm">
          <i class="bi bi-plus-lg"></i> Создать
        </router-link>
        <button v-if="auth.can('import_export')" class="btn btn-outline-primary btn-sm" @click="showImport = true">
          <i class="bi bi-upload"></i> Импорт
        </button>
        <div v-if="auth.can('import_export')" class="btn-group">
          <button class="btn btn-outline-success btn-sm dropdown-toggle" data-bs-toggle="dropdown">
            <i class="bi bi-download"></i> Экспорт
          </button>
          <ul class="dropdown-menu">
            <li><a class="dropdown-item" :href="`/api/projects/${projectId}/export/xlsx`">Excel (.xlsx)</a></li>
            <li><a class="dropdown-item" :href="`/api/projects/${projectId}/export/csv`">CSV</a></li>
            <li><a class="dropdown-item" :href="`/api/projects/${projectId}/export/json`">JSON</a></li>
          </ul>
        </div>
      </div>
    </div>

    <!-- Фильтры -->
    <div class="card mb-3">
      <div class="card-body">
        <div class="row g-2 align-items-end">
          <div class="col-md-3">
            <label class="form-label">Поиск</label>
            <input v-model="filters.q" type="text" class="form-control form-control-sm"
                   placeholder="ID, название, текст..." @keyup.enter="load">
          </div>
          <div class="col-md-2">
            <label class="form-label">Статус</label>
            <select v-model="filters.status" class="form-select form-select-sm" @change="load">
              <option value="">Все</option>
              <option v-for="s in statuses" :key="s" :value="s">{{ s }}</option>
            </select>
          </div>
          <div class="col-md-2">
            <label class="form-label">Приоритет</label>
            <select v-model="filters.priority" class="form-select form-select-sm" @change="load">
              <option value="">Все</option>
              <option v-for="p in priorities" :key="p" :value="p">{{ p }}</option>
            </select>
          </div>
          <div class="col-md-2">
            <label class="form-label">Категория</label>
            <select v-model="filters.category" class="form-select form-select-sm" @change="load">
              <option value="">Все</option>
              <option v-for="c in categories" :key="c" :value="c">{{ c }}</option>
            </select>
          </div>
          <div class="col-md-2">
            <label class="form-label">Сортировка</label>
            <select v-model="filters.sort_by" class="form-select form-select-sm" @change="load">
              <option value="updated_at">Дата изменения</option>
              <option value="created_at">Дата создания</option>
              <option value="priority">Приоритет</option>
              <option value="title">Название</option>
            </select>
          </div>
          <div class="col-md-1">
            <button class="btn btn-sm btn-outline-primary w-100" @click="load">
              <i class="bi bi-search"></i>
            </button>
          </div>
        </div>
      </div>
    </div>

    <!-- Таблица -->
    <div v-if="requirements.length" class="table-responsive">
      <table class="table table-hover table-sm">
        <thead class="table-light">
          <tr>
            <th>ID</th><th>Польз. ID</th><th>Название</th><th>Статус</th>
            <th>Приоритет</th><th>Категория</th><th>Версия</th><th>Ответственный</th>
            <th>Обновлено</th><th>Действия</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="req in requirements" :key="req.id" :class="{ 'table-danger': req.is_deleted }">
            <td><code>{{ req.system_id }}</code></td>
            <td>{{ req.custom_id || '—' }}</td>
            <td>
              <router-link :to="`/projects/${projectId}/requirements/${req.id}`">
                {{ truncate(req.title, 50) }}
              </router-link>
              <span v-if="req.is_baseline" class="badge bg-info ms-1">Baseline</span>
            </td>
            <td><span :class="statusBadge(req.status)">{{ req.status }}</span></td>
            <td><span :class="priorityBadge(req.priority)">{{ req.priority }}</span></td>
            <td>{{ req.category }}</td>
            <td>v{{ req.version }}</td>
            <td>{{ req.responsible_user || '—' }}</td>
            <td>{{ formatDate(req.updated_at) }}</td>
            <td>
              <div class="btn-group btn-group-sm">
                <router-link v-if="auth.can('edit') && !req.is_deleted"
                  :to="`/projects/${projectId}/requirements/${req.id}/edit`"
                  class="btn btn-outline-primary" title="Редактировать">
                  <i class="bi bi-pencil"></i>
                </router-link>
                <button v-if="auth.can('delete') && !req.is_deleted"
                  class="btn btn-outline-danger" @click="deleteReq(req)" title="Удалить">
                  <i class="bi bi-trash"></i>
                </button>
              </div>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
    <div v-else class="text-center py-5">
      <i class="bi bi-inbox display-1 text-muted"></i>
      <p class="mt-3 text-muted">Требований пока нет</p>
    </div>

    <!-- Пагинация -->
    <nav v-if="totalPages > 1">
      <ul class="pagination pagination-sm justify-content-center">
        <li v-for="p in totalPages" :key="p" :class="{ 'page-item': true, active: p === page }">
          <a class="page-link" href="#" @click.prevent="page = p; load()">{{ p }}</a>
        </li>
      </ul>
    </nav>

    <!-- Навигация проекта -->
    <hr>
    <div class="btn-group btn-group-sm">
      <router-link :to="`/projects/${projectId}/tracelinks`" class="btn btn-outline-info">
        <i class="bi bi-diagram-2"></i> Связи трассировки
      </router-link>
      <router-link :to="`/projects/${projectId}/baselines`" class="btn btn-outline-secondary">
        <i class="bi bi-bookmark"></i> Базовые версии
      </router-link>
      <router-link :to="`/projects/${projectId}/change-requests`" class="btn btn-outline-warning">
        <i class="bi bi-pencil-square"></i> Запросы на изменение
      </router-link>
    </div>

    <!-- Импорт модал -->
    <div v-if="showImport" class="modal d-block" style="background:rgba(0,0,0,0.5)" @click.self="showImport=false">
      <div class="modal-dialog">
        <div class="modal-content">
          <div class="modal-header">
            <h5>Импорт данных</h5>
            <button type="button" class="btn-close" @click="showImport=false"></button>
          </div>
          <div class="modal-body">
            <input type="file" ref="importFile" class="form-control" accept=".xlsx,.csv,.json">
            <small class="text-muted">Форматы: XLSX, CSV, JSON</small>
          </div>
          <div class="modal-footer">
            <button class="btn btn-secondary" @click="showImport=false">Отмена</button>
            <button class="btn btn-primary" @click="doImport" :disabled="importing">
              {{ importing ? 'Импорт...' : 'Импортировать' }}
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useAuthStore } from '../stores/auth'
import api from '../api/client'

const props = defineProps(['projectId'])
const emit = defineEmits(['alert'])
const auth = useAuthStore()

const project = ref(null)
const requirements = ref([])
const page = ref(1)
const totalPages = ref(1)
const showImport = ref(false)
const importing = ref(false)
const importFile = ref(null)

const statuses = ['draft','active','approved','implemented','verified','deprecated']
const priorities = ['critical','high','medium','low']
const categories = ['functional','non_functional','interface','performance','security','derived']

const filters = reactive({ q: '', status: '', priority: '', category: '', sort_by: 'updated_at' })

function truncate(s, n) { return s && s.length > n ? s.slice(0, n) + '...' : s }
function formatDate(iso) {
    if (!iso) return '—'
    return new Date(iso).toLocaleString('ru-RU', { day: '2-digit', month: '2-digit', year: 'numeric', hour: '2-digit', minute: '2-digit' })
}
function statusBadge(s) {
    const m = { approved: 'badge bg-success', draft: 'badge bg-warning', deleted: 'badge bg-danger' }
    return m[s] || 'badge bg-secondary'
}
function priorityBadge(p) {
    const m = { critical: 'badge bg-danger', high: 'badge bg-warning', medium: 'badge bg-info' }
    return m[p] || 'badge bg-secondary'
}

async function load() {
    const params = { page: page.value, ...filters }
    Object.keys(params).forEach(k => { if (!params[k]) delete params[k] })
    const { data } = await api.get(`/projects/${props.projectId}/requirements`, { params })
    requirements.value = data.items
    totalPages.value = data.pages
}

async function loadProject() {
    const { data } = await api.get(`/projects/${props.projectId}`)
    project.value = data
}

async function deleteReq(req) {
    if (!confirm(`Удалить требование ${req.system_id}?`)) return
    try {
        await api.delete(`/projects/${props.projectId}/requirements/${req.id}`)
        emit('alert', 'Требование удалено', 'success')
        load()
    } catch (e) {
        emit('alert', e.response?.data?.error || 'Ошибка', 'danger')
    }
}

async function doImport() {
    const file = importFile.value?.files?.[0]
    if (!file) return
    importing.value = true
    const fd = new FormData()
    fd.append('file', file)
    try {
        const { data } = await api.post(`/projects/${props.projectId}/import`, fd, {
            headers: { 'Content-Type': 'multipart/form-data' }
        })
        showImport.value = false
        emit('alert', `Импортировано: ${data.imported_requirements || 0} требований`, 'success')
        load()
    } catch (e) {
        emit('alert', e.response?.data?.error || 'Ошибка импорта', 'danger')
    } finally {
        importing.value = false
    }
}

onMounted(() => { loadProject(); load() })
</script>
