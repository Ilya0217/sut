<template>
  <div class="row justify-content-center mt-5">
    <div class="col-md-4">
      <div class="card shadow">
        <div class="card-body">
          <h3 class="card-title text-center mb-4">
            <i class="bi bi-diagram-3 text-primary"></i><br>
            Система управления требованиями
          </h3>
          <div v-if="error" class="alert alert-danger">{{ error }}</div>
          <form @submit.prevent="doLogin">
            <div class="mb-3">
              <label class="form-label">Имя пользователя</label>
              <input v-model="form.username" type="text" class="form-control" required autofocus>
            </div>
            <div class="mb-3">
              <label class="form-label">Пароль</label>
              <input v-model="form.password" type="password" class="form-control" required>
            </div>
            <button type="submit" class="btn btn-primary w-100" :disabled="loading">
              {{ loading ? 'Вход...' : 'Войти' }}
            </button>
          </form>
          <hr>
          <p class="text-center mb-0">
            <router-link to="/register">Зарегистрироваться</router-link>
          </p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'

const auth = useAuthStore()
const router = useRouter()
const form = reactive({ username: '', password: '' })
const error = ref('')
const loading = ref(false)

async function doLogin() {
    loading.value = true
    error.value = ''
    try {
        await auth.login(form.username, form.password)
        router.push('/')
    } catch (e) {
        error.value = e.response?.data?.error || 'Ошибка входа'
    } finally {
        loading.value = false
    }
}
</script>
