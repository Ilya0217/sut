import { createRouter, createWebHistory } from 'vue-router'
import { useAuthStore } from '../stores/auth'

import LoginView from '../views/LoginView.vue'
import RegisterView from '../views/RegisterView.vue'
import ProjectsView from '../views/ProjectsView.vue'
import RequirementsView from '../views/RequirementsView.vue'
import RequirementDetail from '../views/RequirementDetail.vue'
import RequirementForm from '../views/RequirementForm.vue'
import TraceLinksView from '../views/TraceLinksView.vue'
import BaselinesView from '../views/BaselinesView.vue'
import ChangeRequestsView from '../views/ChangeRequestsView.vue'
import NotificationsView from '../views/NotificationsView.vue'
import AuditView from '../views/AuditView.vue'
import UsersView from '../views/UsersView.vue'

const routes = [
    { path: '/login', name: 'login', component: LoginView, meta: { guest: true } },
    { path: '/register', name: 'register', component: RegisterView, meta: { guest: true } },
    { path: '/', name: 'projects', component: ProjectsView },
    { path: '/projects/:projectId/requirements', name: 'requirements', component: RequirementsView, props: true },
    { path: '/projects/:projectId/requirements/new', name: 'requirement-new', component: RequirementForm, props: true },
    { path: '/projects/:projectId/requirements/:reqId', name: 'requirement-detail', component: RequirementDetail, props: true },
    { path: '/projects/:projectId/requirements/:reqId/edit', name: 'requirement-edit', component: RequirementForm, props: true },
    { path: '/projects/:projectId/tracelinks', name: 'tracelinks', component: TraceLinksView, props: true },
    { path: '/projects/:projectId/baselines', name: 'baselines', component: BaselinesView, props: true },
    { path: '/projects/:projectId/change-requests', name: 'change-requests', component: ChangeRequestsView, props: true },
    { path: '/notifications', name: 'notifications', component: NotificationsView },
    { path: '/audit', name: 'audit', component: AuditView },
    { path: '/users', name: 'users', component: UsersView },
]

const router = createRouter({
    history: createWebHistory(),
    routes,
})

router.beforeEach(async (to) => {
    const auth = useAuthStore()
    if (!auth.user && !to.meta.guest) {
        try {
            await auth.fetchUser()
        } catch {
            // ignore
        }
    }
    if (!to.meta.guest && !auth.user) {
        return { name: 'login' }
    }
})

export default router
