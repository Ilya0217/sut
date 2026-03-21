<template>
  <div>
    <nav aria-label="breadcrumb">
      <ol class="breadcrumb">
        <li class="breadcrumb-item"><router-link to="/">Проекты</router-link></li>
        <li class="breadcrumb-item">
          <router-link :to="`/projects/${projectId}/requirements`">Требования</router-link>
        </li>
        <li class="breadcrumb-item active">Связи трассировки</li>
      </ol>
    </nav>

    <div class="d-flex justify-content-between align-items-center mb-3">
      <h2><i class="bi bi-diagram-2"></i> Связи трассировки</h2>
      <div class="btn-group btn-group-sm">
        <button v-if="auth.can('manage_links')" class="btn btn-primary" @click="showForm = true">
          <i class="bi bi-plus-lg"></i> Создать связь
        </button>
        <button v-if="auth.can('integrity_check')" class="btn btn-warning" @click="runCheck" :disabled="checking">
          <i class="bi bi-shield-check"></i> {{ checking ? 'Проверка...' : 'Проверка целостности' }}
        </button>
      </div>
    </div>

    <div v-if="links.length" class="table-responsive">
      <table class="table table-hover table-sm">
        <thead class="table-light">
          <tr><th>#</th><th>Исходное</th><th></th><th>Целевое</th><th>Тип</th><th>Описание</th><th>Статус</th><th>Дата</th><th>Действия</th></tr>
        </thead>
        <tbody>
          <tr v-for="link in links" :key="link.id" :class="{ 'table-warning': link.status === 'NEEDS_REVIEW' }">
            <td>{{ link.id }}</td>
            <td>
              <router-link :to="`/projects/${projectId}/requirements/${link.source_req_id}`">{{ link.source_system_id }}</router-link>
              <small class="text-muted d-block">{{ (link.source_title || '').slice(0, 25) }}</small>
            </td>
            <td><i class="bi bi-arrow-right"></i></td>
            <td>
              <router-link :to="`/projects/${projectId}/requirements/${link.target_req_id}`">{{ link.target_system_id }}</router-link>
              <small class="text-muted d-block">{{ (link.target_title || '').slice(0, 25) }}</small>
            </td>
            <td><span class="badge bg-primary">{{ link.link_type }}</span></td>
            <td>{{ (link.description || '').slice(0, 30) || '—' }}</td>
            <td><span :class="link.status === 'NEEDS_REVIEW' ? 'badge bg-danger' : 'badge bg-success'">{{ link.status }}</span></td>
            <td>{{ formatDate(link.created_at) }}</td>
            <td>
              <button v-if="auth.can('manage_links')" class="btn btn-outline-danger btn-sm"
                @click="deleteLink(link)" title="Удалить"><i class="bi bi-trash"></i></button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
    <div v-else class="text-center py-5"><p class="text-muted">Связей пока нет</p></div>

    <!-- Модал создания -->
    <div v-if="showForm" class="modal d-block" style="background:rgba(0,0,0,0.5)" @click.self="showForm=false">
      <div class="modal-dialog">
        <div class="modal-content">
          <form @submit.prevent="createLink">
            <div class="modal-header"><h5>Новая связь</h5><button type="button" class="btn-close" @click="showForm=false"></button></div>
            <div class="modal-body">
              <div class="mb-3">
                <label class="form-label">Исходное требование *</label>
                <select v-model="newLink.source_req_id" class="form-select" required>
                  <option value="">Выберите...</option>
                  <option v-for="r in reqs" :key="r.id" :value="r.id">{{ r.system_id }} — {{ r.title.slice(0, 50) }}</option>
                </select>
              </div>
              <div class="mb-3">
                <label class="form-label">Целевое требование *</label>
                <select v-model="newLink.target_req_id" class="form-select" required>
                  <option value="">Выберите...</option>
                  <option v-for="r in reqs" :key="r.id" :value="r.id">{{ r.system_id }} — {{ r.title.slice(0, 50) }}</option>
                </select>
              </div>
              <div class="mb-3">
                <label class="form-label">Тип связи</label>
                <select v-model="newLink.link_type" class="form-select">
                  <option v-for="t in linkTypes" :key="t" :value="t">{{ t }}</option>
                </select>
              </div>
              <div class="mb-3">
                <label class="form-label">Описание</label>
                <textarea v-model="newLink.description" class="form-control" rows="2"></textarea>
              </div>
              <div v-if="formError" class="alert alert-danger">{{ formError }}</div>
            </div>
            <div class="modal-footer">
              <button type="button" class="btn btn-secondary" @click="showForm=false">Отмена</button>
              <button type="submit" class="btn btn-primary">Создать</button>
            </div>
          </form>
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
const links = ref([])
const reqs = ref([])
const showForm = ref(false)
const formError = ref('')
const checking = ref(false)
const linkTypes = ['derives_from', 'satisfies', 'verifies', 'implements']
const newLink = reactive({ source_req_id: '', target_req_id: '', link_type: 'derives_from', description: '' })

function formatDate(iso) { return iso ? new Date(iso).toLocaleDateString('ru-RU') : '—' }

async function load() {
    const [linksRes, reqsRes] = await Promise.all([
        api.get(`/projects/${props.projectId}/tracelinks`),
        api.get(`/projects/${props.projectId}/requirements`, { params: { per_page: 500 } }),
    ])
    links.value = linksRes.data
    reqs.value = reqsRes.data.items
}

async function createLink() {
    formError.value = ''
    try {
        await api.post(`/projects/${props.projectId}/tracelinks`, newLink)
        showForm.value = false
        emit('alert', 'Связь создана', 'success')
        load()
    } catch (e) { formError.value = e.response?.data?.error || 'Ошибка' }
}

async function deleteLink(link) {
    if (!confirm('Удалить связь?')) return
    await api.delete(`/projects/${props.projectId}/tracelinks/${link.id}`)
    load()
}

async function runCheck() {
    checking.value = true
    try {
        const { data } = await api.post(`/projects/${props.projectId}/tracelinks/integrity_check`)
        emit('alert', data.issues_count ? `Найдено проблем: ${data.issues_count}` : 'Нарушений не обнаружено',
             data.issues_count ? 'warning' : 'success')
        load()
    } finally { checking.value = false }
}

onMounted(load)
</script>
