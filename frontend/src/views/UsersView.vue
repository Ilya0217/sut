<template>
  <div>
    <h2><i class="bi bi-people"></i> Управление пользователями</h2>
    <div class="table-responsive">
      <table class="table table-hover">
        <thead class="table-light">
          <tr><th>ID</th><th>Имя</th><th>Email</th><th>Роль</th><th>Зарегистрирован</th><th>Действия</th></tr>
        </thead>
        <tbody>
          <tr v-for="user in users" :key="user.id">
            <td>{{ user.id }}</td>
            <td>{{ user.username }}</td>
            <td>{{ user.email }}</td>
            <td>
              <span :class="roleBadge(user.role)">{{ user.role }}</span>
            </td>
            <td>{{ formatDate(user.created_at) }}</td>
            <td>
              <div class="d-flex gap-1">
                <select v-model="user._newRole" class="form-select form-select-sm" style="width:auto">
                  <option v-for="r in roles" :key="r" :value="r">{{ r }}</option>
                </select>
                <button class="btn btn-sm btn-outline-primary" @click="updateRole(user)">Сохранить</button>
              </div>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import api from '../api/client'

const emit = defineEmits(['alert'])
const users = ref([])
const roles = ['analyst', 'reviewer', 'admin']

function formatDate(iso) { return iso ? new Date(iso).toLocaleDateString('ru-RU') : '—' }
function roleBadge(r) {
    const m = { admin:'badge bg-danger', analyst:'badge bg-primary', reviewer:'badge bg-info' }
    return m[r] || 'badge bg-secondary'
}

async function load() {
    const { data } = await api.get('/users')
    users.value = data.map(u => ({ ...u, _newRole: u.role }))
}

async function updateRole(user) {
    try {
        await api.put(`/users/${user.id}/role`, { role: user._newRole })
        user.role = user._newRole
        emit('alert', 'Роль обновлена', 'success')
    } catch (e) { emit('alert', e.response?.data?.error || 'Ошибка', 'danger') }
}

onMounted(load)
</script>
