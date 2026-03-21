<template>
  <div>
    <div class="d-flex justify-content-between align-items-center mb-3">
      <h2><i class="bi bi-bell"></i> Уведомления</h2>
      <div class="btn-group btn-group-sm">
        <button :class="{'btn':true, 'btn-primary':!unreadOnly, 'btn-outline-primary':unreadOnly}" @click="unreadOnly=false; load()">Все</button>
        <button :class="{'btn':true, 'btn-primary':unreadOnly, 'btn-outline-primary':!unreadOnly}" @click="unreadOnly=true; load()">Непрочитанные</button>
      </div>
    </div>

    <div v-if="items.length" class="list-group">
      <div v-for="n in items" :key="n.id"
           :class="['list-group-item d-flex justify-content-between align-items-start', n.is_read ? '' : 'list-group-item-warning']">
        <div>
          <span class="badge bg-secondary me-1">{{ n.event_type }}</span>
          {{ n.message }}
          <br><small class="text-muted">{{ formatDate(n.created_at) }}</small>
        </div>
        <button v-if="!n.is_read" class="btn btn-sm btn-outline-primary" @click="markRead(n.id)">
          <i class="bi bi-check2"></i>
        </button>
      </div>
    </div>
    <div v-else class="text-center py-5"><p class="text-muted">Уведомлений нет</p></div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import api from '../api/client'

const items = ref([])
const unreadOnly = ref(false)

function formatDate(iso) {
    return iso ? new Date(iso).toLocaleString('ru-RU', { day:'2-digit', month:'2-digit', year:'numeric', hour:'2-digit', minute:'2-digit' }) : '—'
}

async function load() {
    const { data } = await api.get('/notifications', { params: { unread: unreadOnly.value ? '1' : '0' } })
    items.value = data.items
}

async function markRead(id) {
    await api.post(`/notifications/${id}/read`)
    load()
}

onMounted(load)
</script>
