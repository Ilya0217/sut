<template>
  <div>
    <nav aria-label="breadcrumb">
      <ol class="breadcrumb">
        <li class="breadcrumb-item"><router-link to="/">Проекты</router-link></li>
        <li class="breadcrumb-item">
          <router-link :to="`/projects/${projectId}/requirements`">Требования</router-link>
        </li>
        <li class="breadcrumb-item active">Базовые версии</li>
      </ol>
    </nav>

    <div class="d-flex justify-content-between align-items-center mb-3">
      <h2><i class="bi bi-bookmark"></i> Базовые версии</h2>
      <button class="btn btn-primary btn-sm" @click="showForm = true"><i class="bi bi-plus-lg"></i> Создать</button>
    </div>

    <div v-if="baselines.length" class="row">
      <div v-for="bl in baselines" :key="bl.id" class="col-md-4 mb-3">
        <div class="card">
          <div class="card-body">
            <h5>{{ bl.name }}</h5>
            <p class="text-muted">{{ bl.description || 'Без описания' }}</p>
            <small class="text-muted">Создано: {{ formatDate(bl.created_at) }}</small>
            <br><small class="text-muted">Требований: {{ bl.requirements_count }}</small>
          </div>
        </div>
      </div>
    </div>
    <div v-else class="text-center py-5"><p class="text-muted">Базовых версий нет</p></div>

    <!-- Модал -->
    <div v-if="showForm" class="modal d-block" style="background:rgba(0,0,0,0.5)" @click.self="showForm=false">
      <div class="modal-dialog modal-lg">
        <div class="modal-content">
          <form @submit.prevent="create">
            <div class="modal-header"><h5>Новая базовая версия</h5><button type="button" class="btn-close" @click="showForm=false"></button></div>
            <div class="modal-body">
              <div class="mb-3">
                <label class="form-label">Название *</label>
                <input v-model="form.name" type="text" class="form-control" required>
              </div>
              <div class="mb-3">
                <label class="form-label">Описание</label>
                <textarea v-model="form.description" class="form-control" rows="2"></textarea>
              </div>
              <div class="mb-3">
                <label class="form-label">Требования *</label>
                <div class="border rounded p-2" style="max-height:250px;overflow-y:auto">
                  <div v-for="r in reqs" :key="r.id" class="form-check">
                    <input class="form-check-input" type="checkbox" :value="r.id" v-model="form.requirement_ids" :id="'bl_req_'+r.id">
                    <label class="form-check-label" :for="'bl_req_'+r.id">
                      <code>{{ r.system_id }}</code> — {{ r.title.slice(0, 60) }}
                    </label>
                  </div>
                </div>
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
import api from '../api/client'

const props = defineProps(['projectId'])
const emit = defineEmits(['alert'])
const baselines = ref([])
const reqs = ref([])
const showForm = ref(false)
const form = reactive({ name: '', description: '', requirement_ids: [] })

function formatDate(iso) { return iso ? new Date(iso).toLocaleDateString('ru-RU') : '—' }

async function load() {
    const [blRes, reqsRes] = await Promise.all([
        api.get(`/projects/${props.projectId}/baselines`),
        api.get(`/projects/${props.projectId}/requirements`, { params: { per_page: 500 } }),
    ])
    baselines.value = blRes.data
    reqs.value = reqsRes.data.items.filter(r => !r.is_baseline)
}

async function create() {
    try {
        await api.post(`/projects/${props.projectId}/baselines`, form)
        showForm.value = false
        emit('alert', 'Базовая версия создана', 'success')
        load()
    } catch (e) { emit('alert', e.response?.data?.error || 'Ошибка', 'danger') }
}

onMounted(load)
</script>
