<template>
  <div class="row justify-content-center mt-5">
    <div class="col-md-4">
      <div class="card shadow">
        <div class="card-body">
          <h3 class="card-title text-center mb-4">Регистрация</h3>
          <div v-if="error" class="alert alert-danger">{{ error }}</div>
          <form @submit.prevent="doRegister">
            <div class="mb-3">
              <label class="form-label">Имя пользователя</label>
              <input v-model="form.username" type="text" class="form-control" required>
            </div>
            <div class="mb-3">
              <label class="form-label">Email</label>
              <input v-model="form.email" type="email" class="form-control" required>
            </div>
            <div class="mb-3">
              <label class="form-label">Пароль</label>
              <input v-model="form.password" type="password" class="form-control" required minlength="6">
            </div>
            <div class="mb-3">
              <label class="form-label">Роль</label>
              <select v-model="form.role" class="form-select">
                <option value="analyst">Аналитик</option>
                <option value="reviewer">Рецензент</option>
                <option value="admin">Администратор</option>
              </select>
            </div>
            <button type="submit" class="btn btn-primary w-100" :disabled="loading">Зарегистрироваться</button>
          </form>
          <hr>
          <p class="text-center mb-0">
            <router-link to="/login">Уже есть аккаунт? Войти</router-link>
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
const form = reactive({ username: '', email: '', password: '', role: 'analyst' })
const error = ref('')
const loading = ref(false)

async function doRegister() {
    loading.value = true
    error.value = ''
    try {
        await auth.register(form)
        router.push('/')
    } catch (e) {
        error.value = e.response?.data?.error || 'Ошибка регистрации'
    } finally {
        loading.value = false
    }
}
</script>
