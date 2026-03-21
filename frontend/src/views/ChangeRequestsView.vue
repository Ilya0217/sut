<template>
  <div>
    <nav aria-label="breadcrumb">
      <ol class="breadcrumb">
        <li class="breadcrumb-item"><router-link to="/">Проекты</router-link></li>
        <li class="breadcrumb-item">
          <router-link :to="`/projects/${projectId}/requirements`">Требования</router-link>
        </li>
        <li class="breadcrumb-item active">Запросы на изменение</li>
      </ol>
    </nav>

    <div class="d-flex justify-content-between align-items-center mb-3">
      <h2><i class="bi bi-pencil-square"></i> Запросы на изменение</h2>
      <button class="btn btn-primary btn-sm" @click="showForm = true"><i class="bi bi-plus-lg"></i> Новый запрос</button>
    </div>

    <div v-if="items.length" class="table-responsive">
      <table class="table table-hover table-sm">
        <thead class="table-light">
          <tr><th>#</th><th>Требование</th><th>Обоснование</th><th>Статус</th><th>Автор</th><th>Назначено</th><th>Дата</th><th>Действия</th></tr>
        </thead>
        <tbody>
          <tr v-for="cr in items" :key="cr.id">
            <td>{{ cr.id }}</td>
            <td>
              <router-link :to="`/projects/${projectId}/requirements/${cr.requirement_id}`">{{ cr.requirement_system_id }}</router-link>
            </td>
            <td>{{ (cr.justification || '').slice(0, 50) }}</td>
            <td><span :class="crBadge(cr.status)">{{ cr.status }}</span></td>
            <td>{{ cr.requested_by }}</td>
            <td>{{ cr.assigned_to || '—' }}</td>
            <td>{{ formatDate(cr.created_at) }}</td>
            <td>
              <template v-if="cr.status === 'pending' && auth.can('approve')">
                <button class="btn btn-success btn-sm me-1" @click="approve(cr.id)"><i class="bi bi-check"></i></button>
                <button class="btn btn-danger btn-sm" @click="reject(cr.id)"><i class="bi bi-x"></i></button>
              </template>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
    <div v-else class="text-center py-5"><p class="text-muted">Запросов нет</p></div>

    <!-- Модал -->
    <div v-if="showForm" class="modal d-block" style="background:rgba(0,0,0,0.5)" @click.self="showForm=false">
      <div class="modal-dialog">
        <div class="modal-content">
          <form @submit.prevent="create">
            <div class="modal-header"><h5>Новый запрос на изменение</h5><button type="button" class="btn-close" @click="showForm=false"></button></div>
            <div class="modal-body">
              <div class="mb-3">
                <label class="form-label">Требование (из базовой версии) *</label>
                <select v-model="form.requirement_id" class="form-select" required>
                  <option value="">Выберите...</option>
                  <option v-for="r in baselineReqs" :key="r.id" :value="r.id">{{ r.system_id }} — {{ r.title.slice(0, 50) }}</option>
                </select>
              </div>
              <div class="mb-3">
                <label class="form-label">Обоснование *</label>
                <textarea v-model="form.justification" class="form-control" rows="3" required></textarea>
              </div>
              <div class="mb-3">
                <label class="form-label">Описание изменений *</label>
                <textarea v-model="form.changes_description" class="form-control" rows="3" required></textarea>
              </div>
              <div class="mb-3">
                <label class="form-label">Назначить рецензенту</label>
                <select v-model="form.assigned_to" class="form-select">
                  <option :value="null">— Не назначать —</option>
                  <option v-for="u in reviewers" :key="u.id" :value="u.id">{{ u.username }} ({{ u.role }})</option>
                </select>
              </div>
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
const items = ref([])
const baselineReqs = ref([])
const reviewers = ref([])
const showForm = ref(false)
const form = reactive({ requirement_id: '', justification: '', changes_description: '', assigned_to: null })

function formatDate(iso) { return iso ? new Date(iso).toLocaleDateString('ru-RU') : '—' }
function crBadge(s) {
    const m = { approved:'badge bg-success', rejected:'badge bg-danger', pending:'badge bg-warning' }
    return m[s] || 'badge bg-secondary'
}

async function load() {
    const [crRes, reqsRes, usersRes] = await Promise.all([
        api.get(`/projects/${props.projectId}/change_requests`),
        api.get(`/projects/${props.projectId}/requirements`, { params: { per_page: 500 } }),
        api.get('/users'),
    ])
    items.value = crRes.data.items
    baselineReqs.value = reqsRes.data.items.filter(r => r.is_baseline)
    reviewers.value = usersRes.data.filter(u => u.role === 'reviewer' || u.role === 'admin')
}

async function create() {
    try {
        await api.post(`/projects/${props.projectId}/change_requests`, form)
        showForm.value = false
        emit('alert', 'Запрос создан', 'success')
        load()
    } catch (e) { emit('alert', e.response?.data?.error || 'Ошибка', 'danger') }
}

async function approve(id) {
    await api.post(`/projects/${props.projectId}/change_requests/${id}/approve`, {})
    load()
}

async function reject(id) {
    const comment = prompt('Причина отклонения:')
    if (comment === null) return
    await api.post(`/projects/${props.projectId}/change_requests/${id}/reject`, { comment })
    load()
}

onMounted(load)
</script>
