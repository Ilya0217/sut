<template>
  <div v-if="req">
    <nav aria-label="breadcrumb">
      <ol class="breadcrumb">
        <li class="breadcrumb-item"><router-link to="/">Проекты</router-link></li>
        <li class="breadcrumb-item">
          <router-link :to="`/projects/${projectId}/requirements`">Требования</router-link>
        </li>
        <li class="breadcrumb-item active">{{ req.system_id }}</li>
      </ol>
    </nav>

    <div class="row">
      <div class="col-md-8">
        <div class="d-flex justify-content-between align-items-start mb-3">
          <div>
            <h2>{{ req.title }}</h2>
            <code>{{ req.system_id }}</code>
            <template v-if="req.custom_id"> / <code>{{ req.custom_id }}</code></template>
            <span v-if="req.is_baseline" class="badge bg-info ms-1">Baseline</span>
            <span v-if="req.is_deleted" class="badge bg-danger ms-1">Удалено</span>
          </div>
          <div class="btn-group btn-group-sm">
            <router-link v-if="auth.can('edit') && !req.is_deleted"
              :to="`/projects/${projectId}/requirements/${reqId}/edit`" class="btn btn-outline-primary">
              <i class="bi bi-pencil"></i> Редактировать
            </router-link>
            <button v-if="req.is_deleted" class="btn btn-outline-success" @click="restore">
              <i class="bi bi-arrow-counterclockwise"></i> Восстановить
            </button>
          </div>
        </div>

        <div class="card mb-3">
          <div class="card-header">Описание</div>
          <div class="card-body"><p>{{ req.text }}</p></div>
        </div>

        <div class="row mb-3">
          <div class="col-md-4">
            <div class="card"><div class="card-body text-center">
              <small class="text-muted">Статус</small><br>
              <span :class="statusBadge(req.status) + ' fs-6'">{{ req.status }}</span>
            </div></div>
          </div>
          <div class="col-md-4">
            <div class="card"><div class="card-body text-center">
              <small class="text-muted">Приоритет</small><br>
              <span :class="priorityBadge(req.priority) + ' fs-6'">{{ req.priority }}</span>
            </div></div>
          </div>
          <div class="col-md-4">
            <div class="card"><div class="card-body text-center">
              <small class="text-muted">Категория</small><br>
              <span class="fs-6">{{ req.category }}</span>
            </div></div>
          </div>
        </div>

        <!-- Связи -->
        <div class="card mb-3">
          <div class="card-header d-flex justify-content-between">
            <span><i class="bi bi-diagram-2"></i> Связи трассировки</span>
            <router-link v-if="auth.can('manage_links')" :to="`/projects/${projectId}/tracelinks`" class="btn btn-sm btn-outline-primary">
              + Добавить
            </router-link>
          </div>
          <div class="card-body">
            <table v-if="links.length" class="table table-sm">
              <thead><tr><th>Направление</th><th>Требование</th><th>Тип</th><th>Статус</th></tr></thead>
              <tbody>
                <tr v-for="link in links" :key="link.id">
                  <td>
                    <template v-if="link.source_req_id == reqId">
                      <i class="bi bi-arrow-right text-primary"></i> Исходящая
                    </template>
                    <template v-else>
                      <i class="bi bi-arrow-left text-success"></i> Входящая
                    </template>
                  </td>
                  <td>
                    <router-link :to="`/projects/${projectId}/requirements/${link.source_req_id == reqId ? link.target_req_id : link.source_req_id}`">
                      {{ link.source_req_id == reqId ? link.target_system_id : link.source_system_id }}
                    </router-link>
                  </td>
                  <td>{{ link.link_type }}</td>
                  <td><span :class="link.status === 'NEEDS_REVIEW' ? 'badge bg-danger' : 'badge bg-success'">{{ link.status }}</span></td>
                </tr>
              </tbody>
            </table>
            <p v-else class="text-muted mb-0">Нет связей</p>
          </div>
        </div>

        <!-- История -->
        <div class="card">
          <div class="card-header"><i class="bi bi-clock-history"></i> История изменений</div>
          <div class="card-body">
            <table v-if="history.length" class="table table-sm">
              <thead><tr><th>Дата</th><th>Пользователь</th><th>Событие</th><th>Атрибут</th><th>Было</th><th>Стало</th></tr></thead>
              <tbody>
                <tr v-for="h in history" :key="h.id">
                  <td>{{ formatDate(h.created_at) }}</td>
                  <td>{{ h.user || '—' }}</td>
                  <td><span class="badge bg-secondary">{{ h.event_type }}</span></td>
                  <td>{{ h.attribute_name || '—' }}</td>
                  <td>{{ truncate(h.old_value, 30) || '—' }}</td>
                  <td>{{ truncate(h.new_value, 30) || '—' }}</td>
                </tr>
              </tbody>
            </table>
            <p v-else class="text-muted mb-0">Нет записей</p>
          </div>
        </div>
      </div>

      <div class="col-md-4">
        <div class="card">
          <div class="card-header">Метаданные</div>
          <ul class="list-group list-group-flush">
            <li class="list-group-item"><strong>Версия:</strong> {{ req.version }}</li>
            <li class="list-group-item"><strong>Создано:</strong> {{ formatDate(req.created_at) }}</li>
            <li class="list-group-item"><strong>Обновлено:</strong> {{ formatDate(req.updated_at) }}</li>
            <li class="list-group-item"><strong>Автор:</strong> {{ req.created_by || '—' }}</li>
            <li class="list-group-item"><strong>Ответственный:</strong> {{ req.responsible_user || 'Не назначен' }}</li>
          </ul>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useAuthStore } from '../stores/auth'
import api from '../api/client'

const props = defineProps(['projectId', 'reqId'])
const emit = defineEmits(['alert'])
const auth = useAuthStore()
const req = ref(null)
const links = ref([])
const history = ref([])

function truncate(s, n) { return s && s.length > n ? s.slice(0, n) + '...' : s }
function formatDate(iso) {
    if (!iso) return '—'
    return new Date(iso).toLocaleString('ru-RU', { day:'2-digit', month:'2-digit', year:'numeric', hour:'2-digit', minute:'2-digit' })
}
function statusBadge(s) {
    const m = { approved:'badge bg-success', draft:'badge bg-warning', deleted:'badge bg-danger' }
    return m[s] || 'badge bg-secondary'
}
function priorityBadge(p) {
    const m = { critical:'badge bg-danger', high:'badge bg-warning', medium:'badge bg-info' }
    return m[p] || 'badge bg-secondary'
}

async function restore() {
    await api.post(`/projects/${props.projectId}/requirements/${props.reqId}/restore`)
    emit('alert', 'Требование восстановлено', 'success')
    load()
}

async function load() {
    const { data } = await api.get(`/projects/${props.projectId}/requirements/${props.reqId}`)
    req.value = data
    const linksRes = await api.get(`/projects/${props.projectId}/tracelinks/requirement/${props.reqId}`)
    links.value = linksRes.data
    const histRes = await api.get(`/projects/${props.projectId}/requirements/${props.reqId}/history`)
    history.value = histRes.data.items
}

onMounted(load)
</script>
