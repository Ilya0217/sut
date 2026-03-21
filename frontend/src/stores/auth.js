import { defineStore } from 'pinia'
import { ref } from 'vue'
import api from '../api/client'

export const useAuthStore = defineStore('auth', () => {
    const user = ref(null)

    async function fetchUser() {
        try {
            const { data } = await api.get('/auth/me')
            user.value = data
        } catch {
            user.value = null
        }
    }

    async function login(username, password) {
        const { data } = await api.post('/auth/login', { username, password })
        user.value = data
        return data
    }

    async function register(form) {
        const { data } = await api.post('/auth/register', form)
        user.value = data
        return data
    }

    async function logout() {
        await api.post('/auth/logout')
        user.value = null
    }

    function can(action) {
        if (!user.value) return false
        const role = user.value.role
        const perms = {
            create: ['analyst', 'admin'],
            edit: ['analyst', 'admin'],
            delete: ['analyst', 'admin'],
            manage_links: ['analyst', 'admin'],
            approve: ['reviewer', 'admin'],
            manage_users: ['admin'],
            import_export: ['analyst', 'admin'],
            integrity_check: ['analyst', 'admin'],
        }
        return (perms[action] || []).includes(role)
    }

    return { user, fetchUser, login, register, logout, can }
})
