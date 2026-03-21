<template>
  <div>
    <nav aria-label="breadcrumb">
      <ol class="breadcrumb">
        <li class="breadcrumb-item"><router-link to="/">Проекты</router-link></li>
        <li class="breadcrumb-item">
          <router-link :to="`/projects/${projectId}/requirements`">Требования</router-link>
        </li>
        <li class="breadcrumb-item active">{{ isEdit ? 'Редактирование' : 'Новое требование' }}</li>
      </ol>
    </nav>

    <div class="row">
      <div class="col-md-8">
        <h2>{{ isEdit ? 'Редактирование требования' : 'Новое требование' }}</h2>
        <form @submit.prevent="save">
          <div class="row mb-3">
            <div class="col-md-6">
              <label class="form-label">Заголовок *</label>
              <input v-model="form.title" type="text" class="form-control" required>
            </div>
            <div class="col-md-6">
              <label class="form-label">Пользовательский ID</label>
              <input v-model="form.custom_id" type="text" class="form-control" placeholder="FR-001">
            </div>
          </div>
          <div class="mb-3">
            <label class="form-label">Текст требования *</label>
            <textarea v-model="form.text" class="form-control" rows="5" required></textarea>
          </div>
          <div class="row mb-3">
            <div class="col-md-4">
              <label class="form-label">Категория *</label>
              <select v-model="form.category" class="form-select" required>
                <option v-for="c in categories" :key="c" :value="c">{{ c }}</option>
              </select>
            </div>
            <div class="col-md-4">
              <label class="form-label">Приоритет *</label>
              <select v-model="form.priority" class="form-select" required>
                <option v-for="p in priorities" :key="p" :value="p">{{ p }}</option>
              </select>
            </div>
            <div class="col-md-4">
              <label class="form-label">Статус *</label>
              <select v-model="form.status" class="form-select" required>
                <option v-for="s in statuses" :key="s" :value="s">{{ s }}</option>
              </select>
            </div>
          </div>
          <div class="row mb-3">
            <div class="col-md-6">
              <label class="form-label">Родительское требование</label>
              <select v-model="form.parent_id" class="form-select">
                <option :value="null">— Нет —</option>
                <option v-for="r in parentReqs" :key="r.id" :value="r.id">
                  {{ r.system_id }} — {{ r.title.slice(0, 40) }}
                </option>
              </select>
            </div>
            <div class="col-md-6">
              <label class="form-label">Ответственное лицо</label>
              <select v-model="form.responsible_user_id" class="form-select">
                <option :value="null">— Не назначено —</option>
                <option v-for="u in users" :key="u.id" :value="u.id">{{ u.username }} ({{ u.role }})</option>
              </select>
            </div>
          </div>
          <div v-if="error" class="alert alert-danger">{{ error }}</div>
          <div class="d-flex gap-2">
            <button type="submit" class="btn btn-primary" :disabled="saving">
              <i class="bi bi-check-lg"></i> {{ isEdit ? 'Сохранить' : 'Создать' }}
            </button>
            <router-link :to="`/projects/${projectId}/requirements`" class="btn btn-secondary">Отмена</router-link>
          </div>
        </form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import api from '../api/client'

const props = defineProps(['projectId', 'reqId'])
const router = useRouter()
const isEdit = computed(() => !!props.reqId)
const error = ref('')
const saving = ref(false)
const users = ref([])
const parentReqs = ref([])

const statuses = ['draft','active','approved','implemented','verified','deprecated']
const priorities = ['critical','high','medium','low']
const categories = ['functional','non_functional','interface','performance','security','derived']

const form = reactive({
    title: '', text: '', category: 'functional', priority: 'medium', status: 'draft',
    custom_id: '', parent_id: null, responsible_user_id: null,
})

async function loadData() {
    const [usersRes, reqsRes] = await Promise.all([
        api.get('/users'),
        api.get(`/projects/${props.projectId}/requirements`, { params: { per_page: 500 } }),
    ])
    users.value = usersRes.data
    parentReqs.value = reqsRes.data.items.filter(r => r.id != props.reqId)

    if (isEdit.value) {
        const { data } = await api.get(`/projects/${props.projectId}/requirements/${props.reqId}`)
        Object.assign(form, {
            title: data.title, text: data.text, category: data.category,
            priority: data.priority, status: data.status, custom_id: data.custom_id || '',
            parent_id: data.parent_id, responsible_user_id: data.responsible_user_id,
        })
    }
}

async function save() {
    saving.value = true
    error.value = ''
    try {
        if (isEdit.value) {
            await api.put(`/projects/${props.projectId}/requirements/${props.reqId}`, form)
            router.push(`/projects/${props.projectId}/requirements/${props.reqId}`)
        } else {
            const { data } = await api.post(`/projects/${props.projectId}/requirements`, form)
            router.push(`/projects/${props.projectId}/requirements/${data.id}`)
        }
    } catch (e) {
        error.value = e.response?.data?.error || 'Ошибка сохранения'
    } finally {
        saving.value = false
    }
}

onMounted(loadData)
</script>
