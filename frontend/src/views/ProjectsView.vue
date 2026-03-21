<template>
  <div>
    <div class="d-flex justify-content-between align-items-center mb-4">
      <h2><i class="bi bi-folder"></i> Проекты</h2>
      <button class="btn btn-primary" data-bs-toggle="modal" data-bs-target="#newProjectModal">
        <i class="bi bi-plus-lg"></i> Новый проект
      </button>
    </div>

    <div v-if="projects.length" class="row">
      <div v-for="p in projects" :key="p.id" class="col-md-4 mb-3">
        <div class="card h-100 shadow-sm">
          <div class="card-body">
            <h5 class="card-title">{{ p.name }}</h5>
            <p class="card-text text-muted">{{ p.description || 'Без описания' }}</p>
            <small class="text-muted">Создан: {{ formatDate(p.created_at) }}</small>
          </div>
          <div class="card-footer bg-transparent">
            <div class="btn-group btn-group-sm w-100">
              <router-link :to="`/projects/${p.id}/requirements`" class="btn btn-outline-primary">
                <i class="bi bi-list-check"></i> Требования
              </router-link>
              <router-link :to="`/projects/${p.id}/tracelinks`" class="btn btn-outline-info">
                <i class="bi bi-diagram-2"></i> Связи
              </router-link>
              <router-link :to="`/projects/${p.id}/baselines`" class="btn btn-outline-secondary">
                <i class="bi bi-bookmark"></i> Версии
              </router-link>
            </div>
          </div>
        </div>
      </div>
    </div>
    <div v-else class="text-center py-5">
      <i class="bi bi-folder-plus display-1 text-muted"></i>
      <p class="mt-3 text-muted">Проектов пока нет. Создайте первый!</p>
    </div>

    <!-- Модал создания проекта -->
    <div class="modal fade" id="newProjectModal" tabindex="-1">
      <div class="modal-dialog">
        <div class="modal-content">
          <form @submit.prevent="createProject">
            <div class="modal-header">
              <h5 class="modal-title">Новый проект</h5>
              <button type="button" class="btn-close" data-bs-dismiss="modal"></button>
            </div>
            <div class="modal-body">
              <div class="mb-3">
                <label class="form-label">Название *</label>
                <input v-model="newProject.name" type="text" class="form-control" required>
              </div>
              <div class="mb-3">
                <label class="form-label">Описание</label>
                <textarea v-model="newProject.description" class="form-control" rows="3"></textarea>
              </div>
            </div>
            <div class="modal-footer">
              <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">Отмена</button>
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
import { useRouter } from 'vue-router'
import api from '../api/client'

const emit = defineEmits(['alert'])
const router = useRouter()
const projects = ref([])
const newProject = reactive({ name: '', description: '' })

function formatDate(iso) {
    if (!iso) return '—'
    return new Date(iso).toLocaleDateString('ru-RU')
}

async function load() {
    const { data } = await api.get('/projects')
    projects.value = data
}

async function createProject() {
    try {
        const { data } = await api.post('/projects', newProject)
        newProject.name = ''
        newProject.description = ''
        bootstrap.Modal.getInstance(document.getElementById('newProjectModal')).hide()
        router.push(`/projects/${data.id}/requirements`)
    } catch (e) {
        emit('alert', e.response?.data?.error || 'Ошибка', 'danger')
    }
}

onMounted(load)
</script>
