<template>
  <div>
    <nav v-if="auth.user" class="navbar navbar-expand-lg navbar-dark bg-primary">
      <div class="container-fluid">
        <router-link class="navbar-brand" to="/">
          <i class="bi bi-diagram-3"></i> СУТ
        </router-link>
        <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarNav">
          <span class="navbar-toggler-icon"></span>
        </button>
        <div class="collapse navbar-collapse" id="navbarNav">
          <ul class="navbar-nav me-auto">
            <li class="nav-item">
              <router-link class="nav-link" to="/"><i class="bi bi-folder"></i> Проекты</router-link>
            </li>
            <li class="nav-item">
              <router-link class="nav-link" to="/audit"><i class="bi bi-journal-text"></i> Аудит</router-link>
            </li>
            <li v-if="auth.can('manage_users')" class="nav-item">
              <router-link class="nav-link" to="/users"><i class="bi bi-people"></i> Пользователи</router-link>
            </li>
          </ul>
          <ul class="navbar-nav">
            <li class="nav-item">
              <router-link class="nav-link position-relative" to="/notifications">
                <i class="bi bi-bell"></i>
                <span v-if="unreadCount > 0"
                      class="position-absolute top-0 start-100 translate-middle badge rounded-pill bg-danger">
                  {{ unreadCount }}
                </span>
              </router-link>
            </li>
            <li class="nav-item dropdown">
              <a class="nav-link dropdown-toggle" href="#" data-bs-toggle="dropdown">
                <i class="bi bi-person-circle"></i> {{ auth.user.username }}
                <span class="badge bg-light text-dark">{{ auth.user.role }}</span>
              </a>
              <ul class="dropdown-menu dropdown-menu-end">
                <li><a class="dropdown-item" href="#" @click.prevent="doLogout">
                  <i class="bi bi-box-arrow-right"></i> Выйти
                </a></li>
              </ul>
            </li>
          </ul>
        </div>
      </div>
    </nav>

    <div class="container-fluid mt-3">
      <div v-if="alert.message" :class="'alert alert-' + alert.type + ' alert-dismissible fade show'">
        {{ alert.message }}
        <button type="button" class="btn-close" @click="alert.message = ''"></button>
      </div>
      <router-view :key="$route.fullPath" @alert="showAlert" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from './stores/auth'
import api from './api/client'

const auth = useAuthStore()
const router = useRouter()
const unreadCount = ref(0)
const alert = ref({ message: '', type: 'info' })

function showAlert(msg, type = 'success') {
    alert.value = { message: msg, type }
    setTimeout(() => { alert.value.message = '' }, 5000)
}

async function fetchUnread() {
    if (!auth.user) return
    try {
        const { data } = await api.get('/notifications/unread_count')
        unreadCount.value = data.count
    } catch { /* ignore */ }
}

async function doLogout() {
    await auth.logout()
    router.push('/login')
}

onMounted(async () => {
    if (auth.user) fetchUnread()
    setInterval(fetchUnread, 30000)
})
</script>
